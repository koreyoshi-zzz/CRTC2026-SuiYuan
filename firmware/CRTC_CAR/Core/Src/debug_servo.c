/*
 * debug_servo.c
 *
 *  Created on: 2026年10月19日
 *      Author: zc
 *
 *  舵机独立调试程序（配合 debug_config.h 的 DEBUG_MODE_SERVO 使用）
 *
 *  ── 怎么看结果 ────────────────────────────────────────────
 *  PC13 上的板载 LED 用来指示"当前跑到第几个测试项"：
 *    进入每一项前，LED 先快闪 N 下（N = 测试项编号），然后开始动作。
 *    全部跑完停 2 秒，再从第 1 项循环。
 *
 *  闪烁次数 → 测试内容：
 *    闪 1 下 → 三路舵机回中 90°，保持 2 秒（确认上电基准位）
 *    闪 2 下 → 舵机1（SG90，PB6）0°→180°→90° 来回扫掠
 *    闪 3 下 → 舵机2（SG90，PB7）0°→180°→90° 来回扫掠
 *    闪 4 下 → 舵机3（MG995，PB8）0°→180°→90° 来回扫掠
 *    闪 5 下 → 三路同步扫掠（检查同时动作时电源是否被拉垮）
 *    闪 6 下 → 三路保持在 45° 不动，持续 3 秒（堵转 / 保持力矩测试）
 *
 *  ── 常见故障对照 ──────────────────────────────────────────
 *  · 舵机完全不动        → 查 TIM4 是否启动、PB6/7/8 接线、舵机 5V 供电
 *  · 单路能动多路不动    → 5V 电源电流不够（MG995 堵转可达 1A 以上）
 *  · 多路同时动时主控复位 → 舵机 5V 与主控 5V 必须分开供电，并加储能电容
 *  · 角度到不了 0/180    → 改 PULSE_MIN_US / PULSE_MAX_US（不同舵机脉宽范围有差异）
 */
#include "debug_config.h"

#if DEBUG_MODE == DEBUG_MODE_SERVO

#include "main.h"
#include "servo.h"
#include "tim.h"

/* PC13 低电平点亮 */
#define LED_ON()     HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define LED_OFF()    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)

/* 快闪 n 下，用来播报测试项编号 */
static void blink(uint8_t times)
{
    for (uint8_t i = 0; i < times; i++) {
        LED_ON();
        HAL_Delay(120);
        LED_OFF();
        HAL_Delay(180);
    }
    HAL_Delay(400);          /* 闪完留个间隔，方便区分"编号"和"动作开始" */
}

/* 单舵机扫掠：从 start 平滑走到 end，每步 step 度、每步延时 delay_ms */
static void sweep(uint8_t id, float start, float end, float step, uint16_t delay_ms)
{
    if (start <= end) {
        for (float a = start; a <= end; a += step) {
            Servo_SetAngle(id, a);
            HAL_Delay(delay_ms);
        }
    } else {
        for (float a = start; a >= end; a -= step) {
            Servo_SetAngle(id, a);
            HAL_Delay(delay_ms);
        }
    }
}

/* 三项扫掠的小封装：去程 + 回程，回到中位 */
static void sweep_and_back(uint8_t id)
{
    sweep(id,  90.0f,   0.0f, 5.0f, 25);    /* 中位 → 0°   */
    sweep(id,   0.0f, 180.0f, 5.0f, 25);    /* 0°   → 180° */
    sweep(id, 180.0f,  90.0f, 5.0f, 25);    /* 180° → 中位 */
}

void Debug_Servo_Run(void)
{
    Servo_Init();              /* 内部已把三路 PWM 打开并全部回中 90° */
    LED_OFF();
    HAL_Delay(800);

    while (1)
    {
        /* ---- 测试 1：三路回中，确认基准位 ---- */
        blink(1);
        Servo_SetAngle(SERVO_SG90_1, 90.0f);
        Servo_SetAngle(SERVO_SG90_2, 90.0f);
        Servo_SetAngle(SERVO_MG995,  90.0f);
        LED_ON();                    /* 常亮表示"正在保持中位" */
        HAL_Delay(2000);
        LED_OFF();
        HAL_Delay(400);

        /* ---- 测试 2：舵机 1（SG90，PB6）单独扫掠 ---- */
        blink(2);
        sweep_and_back(SERVO_SG90_1);
        HAL_Delay(400);

        /* ---- 测试 3：舵机 2（SG90，PB7）单独扫掠 ---- */
        blink(3);
        sweep_and_back(SERVO_SG90_2);
        HAL_Delay(400);

        /* ---- 测试 4：舵机 3（MG995，PB8）单独扫掠 ---- */
        blink(4);
        sweep_and_back(SERVO_MG995);
        HAL_Delay(400);

        /* ---- 测试 5：三路同步扫掠（重点看电源扛不扛得住） ---- */
        blink(5);
        for (float a = 45.0f; a <= 135.0f; a += 5.0f) {   /* 同步正扫 */
            Servo_SetAngle(SERVO_SG90_1, a);
            Servo_SetAngle(SERVO_SG90_2, a);
            Servo_SetAngle(SERVO_MG995,  a);
            HAL_Delay(30);
        }
        for (float a = 135.0f; a >= 45.0f; a -= 5.0f) {   /* 同步回扫 */
            Servo_SetAngle(SERVO_SG90_1, a);
            Servo_SetAngle(SERVO_SG90_2, a);
            Servo_SetAngle(SERVO_MG995,  a);
            HAL_Delay(30);
        }
        HAL_Delay(400);

        /* ---- 测试 6：三路保持 45° 不动（保持力矩 / 堵转发热观察） ---- */
        blink(6);
        Servo_SetAngle(SERVO_SG90_1, 45.0f);
        Servo_SetAngle(SERVO_SG90_2, 45.0f);
        Servo_SetAngle(SERVO_MG995,  45.0f);
        LED_ON();
        HAL_Delay(3000);              /* 保持 3 秒，期间可用手轻掰舵盘感受力矩 */
        LED_OFF();
        HAL_Delay(400);

        /* 一轮跑完，停 2 秒后循环，方便反复观察 */
        Servo_SetAngle(SERVO_SG90_1, 90.0f);
        Servo_SetAngle(SERVO_SG90_2, 90.0f);
        Servo_SetAngle(SERVO_MG995,  90.0f);
        HAL_Delay(2000);
    }
}

#endif /* DEBUG_MODE == DEBUG_MODE_SERVO */
