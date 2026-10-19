/*
 * debug_ps2.c
 *
 *  Created on: 2026年10月19日
 *      Author: zc
 *
 *  PS2 手柄独立调试程序（配合 debug_config.h 的 DEBUG_MODE_PS2 使用）
 *
 *  ── 怎么看结果 ────────────────────────────────────────────
 *  板子上只有 PC13 一个 LED，没有串口，所以全部用【闪烁节奏】编码状态。
 *  进入每项测试前，LED 先快闪 N 下播报编号：
 *
 *    闪 1 下 → 在线检测：连上后 LED 常亮；掉线则急促快闪
 *    闪 2 下 → 按键测试：按住任意键 LED 亮，松开灭（逐个键验证）
 *    闪 3 下 → 左摇杆 X（左右推）：偏离中位越大，闪得越快
 *    闪 4 下 → 左摇杆 Y（上下推）：同上
 *    闪 5 下 → 右摇杆 X（左右推）：同上
 *    闪 6 下 → 右摇杆 Y（上下推）：同上
 *    闪 7 下 → 解锁条件测试：按住 START（且 R1 松开、四摇杆归中）→ LED 常亮
 *
 *  ── 常见故障对照 ──────────────────────────────────────────
 *  · LED 一直快闪（连不上）
 *      → 接收器没插好 / 手柄没开机 / 没自动对频（等接收器灯由快转变常亮）
 *      → 查 PB12~PB15 四根线；山寨接收器对 3.3V 供电敏感，确认电压够
 *  · 按键位图全乱
 *      → 时序太快，把 ps2.c 里 delay_us(4) 调到 6~8 再试
 *  · 摇杆值乱跳
 *      → ps2.c 里已丢弃全 0 / 全 FF 脏帧；如仍跳，检查 DAT 线上拉
 *  · 摇杆推到底数值到不了 0 或 255
 *      → 正常现象，真实摇杆留有余量，通常落在 5~250 之间
 */
#include "debug_config.h"

#if DEBUG_MODE == DEBUG_MODE_PS2

#include "main.h"
#include "ps2.h"

#define LED_ON()     HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define LED_OFF()    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)

/* 快闪 n 下，播报测试项编号 */
static void blink(uint8_t times)
{
    for (uint8_t i = 0; i < times; i++) {
        LED_ON();
        HAL_Delay(120);
        LED_OFF();
        HAL_Delay(180);
    }
    HAL_Delay(400);
}

/* 把摇杆偏离中位的程度（0~127）映射成 LED 闪烁间隔（ms）
 *   偏离 0   → 600ms（慢闪，代表回中）
 *   偏离 127 →  60ms（急闪，代表推到底）
 * 这样"推得越多、闪得越快"，肉眼就能判断摇杆有没有响应、是否线性。 */
static uint16_t offset_to_interval(uint8_t v)
{
    int16_t d = (int16_t)v - 128;
    if (d < 0) d = -d;                 /* 取绝对值 */
    if (d > 127) d = 127;
    return (uint16_t)(600 - (d * 540 / 127));
}

void Debug_PS2_Run(void)
{
    PS2_Init();                 /* 内部含 500ms 上电等待与配置命令序列 */
    PS2_State_t ps2;
    LED_OFF();
    HAL_Delay(500);

    while (1)
    {
        /* ---- 测试 1：在线检测 ---- */
        blink(1);
        {
            uint32_t t0 = HAL_GetTick();
            while (HAL_GetTick() - t0 < 5000) {          /* 观察 5 秒 */
                if (PS2_Poll(&ps2)) {
                    LED_ON();                            /* 连上：常亮 */
                } else {
                    LED_ON();  HAL_Delay(60);            /* 掉线：急闪 */
                    LED_OFF(); HAL_Delay(60);
                }
                HAL_Delay(15);
            }
        }
        LED_OFF();
        HAL_Delay(400);

        /* ---- 测试 2：按键测试（任意键按下即亮） ---- */
        blink(2);
        {
            uint32_t t0 = HAL_GetTick();
            while (HAL_GetTick() - t0 < 8000) {          /* 给 8 秒，够把 16 个键按一遍 */
                if (PS2_Poll(&ps2)) {
                    if (ps2.buttons) LED_ON();           /* 有键按下：亮 */
                    else             LED_OFF();          /* 全松开：灭 */
                } else {
                    LED_ON();  HAL_Delay(60);            /* 掉线急闪提示 */
                    LED_OFF(); HAL_Delay(60);
                }
                HAL_Delay(15);
            }
        }
        LED_OFF();
        HAL_Delay(400);

        /* ---- 测试 3~6：四个摇杆通道依次观察，每个 5 秒 ---- */
        for (uint8_t axis = 0; axis < 4; axis++) {
            blink(3 + axis);                              /* 依次闪 3/4/5/6 下 */
            uint32_t t0 = HAL_GetTick();
            while (HAL_GetTick() - t0 < 5000) {
                if (PS2_Poll(&ps2)) {
                    uint8_t v;
                    switch (axis) {
                        case 0:  v = ps2.lx; break;       /* 左摇杆 X */
                        case 1:  v = ps2.ly; break;       /* 左摇杆 Y */
                        case 2:  v = ps2.rx; break;       /* 右摇杆 X */
                        default: v = ps2.ry; break;       /* 右摇杆 Y */
                    }
                    uint16_t iv = offset_to_interval(v);
                    LED_ON();
                    HAL_Delay(iv / 2);
                    LED_OFF();
                    HAL_Delay(iv / 2);
                } else {
                    LED_ON();  HAL_Delay(60);
                    LED_OFF(); HAL_Delay(60);
                }
            }
            LED_OFF();
            HAL_Delay(400);
        }

        /* ---- 测试 7：解锁条件测试 ----
         * 正式程序的安全逻辑：START 按下 + R1 松开 + 四摇杆全部归中 → 解锁
         * 松开 START 或推任意一个摇杆 → 立刻重新上锁 */
        blink(7);
        {
            uint8_t unlocked = 0;
            uint16_t last_btn = 0;
            uint32_t t0 = HAL_GetTick();
            while (HAL_GetTick() - t0 < 10000) {          /* 观察 10 秒 */
                if (PS2_Poll(&ps2)) {
                    uint16_t pressed = ps2.buttons & (uint16_t)(~last_btn);

                    if (pressed & PS2_BTN_START) {
                        uint8_t centered =
                            (ps2.lx > 116 && ps2.lx < 140 &&
                             ps2.ly > 116 && ps2.ly < 140 &&
                             ps2.rx > 116 && ps2.rx < 140 &&
                             ps2.ry > 116 && ps2.ry < 140);
                        if (!(ps2.buttons & PS2_BTN_R1) && centered) {
                            unlocked = 1;                  /* 满足解锁条件 */
                        }
                    }
                    if (pressed & PS2_BTN_SELECT) unlocked = 0;
                    if (ps2.buttons & PS2_BTN_R1)  unlocked = 0;   /* 按住 R1 时不允许解锁 */

                    if (unlocked) LED_ON();                /* 已解锁：常亮 */
                    else {
                        LED_ON();  HAL_Delay(150);         /* 未解锁：慢闪 */
                        LED_OFF(); HAL_Delay(150);
                    }
                    last_btn = ps2.buttons;
                } else {
                    unlocked = 0;                          /* 掉线重新上锁 */
                    LED_ON();  HAL_Delay(60);
                    LED_OFF(); HAL_Delay(60);
                    last_btn = 0;
                }
                HAL_Delay(15);
            }
        }
        LED_OFF();
        HAL_Delay(1000);
    }
}

#endif /* DEBUG_MODE == DEBUG_MODE_PS2 */
