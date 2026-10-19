/*
 * ps2.c
 *
 *  Created on: 2026年10月15日
 *      Author: zc
 */
#include "ps2.h"
#include "main.h"

static void DWT_Init(void){
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* 使能内核跟踪单元（DWT 的总开关） */
	DWT->CYCCNT = 0;                                 /* 计数器清零，从零开始数 */
	DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}
static void delay_us(uint32_t us){
    uint32_t start = DWT->CYCCNT;                         /* 记下进入时的计数值 */
    uint32_t ticks = us * (SystemCoreClock / 1000000U);   /* 延时换算成周期数（72MHz → us×72） */
    while ((DWT->CYCCNT - start) < ticks) {}             /* 原地等到计数器走过 ticks 个周期 */
}
static uint8_t PS2_Transfer(uint8_t tx){
    uint8_t rx = 0;                                  /* 接收缓冲，逐位拼装 */
    for (int i = 0; i < 8; i++)                      /* 一个字节 8 位，循环 8 次 */
    {
        HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_RESET);   /* ① CLK 拉低 */
        HAL_GPIO_WritePin(PS2_CMD_GPIO_Port, PS2_CMD_Pin,(tx & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET); /*    把 tx 的 bit0 放到 CMD 线 */
        tx >>= 1;                                    /*    tx 右移一位，准备发下一位 */
        delay_us(4);                                 /*    等手柄准备好（建立时间） */

        HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);     /* ② CLK 拉高，手柄输出数据 */
        delay_us(4);                                 /*    给手柄 4us 把数据放到 DAT 线上 */
        if (HAL_GPIO_ReadPin(PS2_DAT_GPIO_Port, PS2_DAT_Pin))                /* ③ 读 DAT */
            rx |= (uint8_t)(1 << i);                 /*    收到 1 就置位（LSB 先行：先进 bit0） */
        delay_us(2);                                 /*    位与位之间的间隔 */
    }
    return rx;                                       /* 返回手柄回传的整个字节 */
}
static void PS2_Cmd(const uint8_t *cmd, uint8_t *resp, uint8_t len){
    HAL_GPIO_WritePin(PS2_CS_GPIO_Port, PS2_CS_Pin, GPIO_PIN_RESET);  /* 选中手柄 */
    delay_us(20);
    for (uint8_t i = 0; i < len; i++){
        uint8_t r = PS2_Transfer(cmd[i]);            /* 发 cmd[i]，同时收一个字节 */
        if (resp) resp[i] = r;                       /* 调用者要应答就存起来，不要就丢弃 */
        delay_us(2);                                 /* 字节之间的间隔 */
    }
    delay_us(20);
    HAL_GPIO_WritePin(PS2_CS_GPIO_Port, PS2_CS_Pin, GPIO_PIN_SET);    /* 释放手柄 */
    HAL_Delay(1);                                    /* 两次事务之间至少歇 1ms */
}
void PS2_Init(void){
    DWT_Init();                                      /* 先启动 us 级延时 */

    HAL_GPIO_WritePin(PS2_CS_GPIO_Port,  PS2_CS_Pin,  GPIO_PIN_SET);   /* CS 空闲为高 */
    HAL_GPIO_WritePin(PS2_CLK_GPIO_Port, PS2_CLK_Pin, GPIO_PIN_SET);   /* CLK 空闲为高 */

    HAL_Delay(500);   /* 等接收器上电并自动对频（灯从快闪变常亮） */

    static const uint8_t enter_cfg[]  = {0x01, 0x43, 0x00, 0x01, 0x00};
    static const uint8_t set_analog[] = {0x01, 0x44, 0x00, 0x01, 0x03,
                                         0x00, 0x00, 0x00, 0x00};
    static const uint8_t motor_map[]  = {0x01, 0x4D, 0x00, 0x00, 0x01,
                                         0xFF, 0xFF, 0xFF, 0xFF};
    static const uint8_t exit_cfg[]   = {0x01, 0x43, 0x00, 0x00, 0x5A,
                                         0x5A, 0x5A, 0x5A, 0x5A};

    PS2_Cmd(enter_cfg,  NULL, 5);   /* 进入配置模式（不需要应答，resp 传 NULL） */
    HAL_Delay(5);                   /* 配置命令之间多歇几 ms，山寨手柄反应慢 */
    PS2_Cmd(set_analog, NULL, 9);   /* 开摇杆模拟量并锁定红灯模式 */
    HAL_Delay(5);
    PS2_Cmd(motor_map,  NULL, 9);   /* 震动马达映射到轮询字节（备用） */
    HAL_Delay(5);
    PS2_Cmd(exit_cfg,   NULL, 9);   /* 退出配置模式 */
    HAL_Delay(10);
}
uint8_t PS2_Poll(PS2_State_t *s)
{
    uint8_t req[9]  = {0x01, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t resp[9] = {0};                           /* 应答缓冲先清零 */

    PS2_Cmd(req, resp, 9);
    if (resp[2] != 0x5A){
      s->connected = 0;                            /* 标记掉线 */
      return 0;                                    /* 告诉主循环：本次数据无效 */
    }

    /* ② 手柄 ID：模拟红灯模式 = 0x73，数字模式 = 0x41。
     *   出现其它值说明这一帧是脏数据（掉线/干扰/手柄正在重启）。 */
    if (resp[1] != 0x73 && resp[1] != 0x41){
    	s->connected = 0;
    	return 0;
    }

    /* ③ 摇杆端点值：真实摇杆不会长时间停在 0 或 255。
     *   四个摇杆【同时】都是端点值 → 一定是总线异常（不是人为推到极限）。 */
    if ((resp[5] == 0x00 || resp[5] == 0xFF) &&
    	(resp[6] == 0x00 || resp[6] == 0xFF) &&
		(resp[7] == 0x00 || resp[7] == 0xFF) &&
		(resp[8] == 0x00 || resp[8] == 0xFF)){
        s->connected = 0;
        return 0;
    }
    s->buttons = (uint16_t)(~(resp[3] | ((uint16_t)resp[4] << 8)));
    s->rx = resp[5];                                 /* 右摇杆 X */
    s->ry = resp[6];                                 /* 右摇杆 Y */
    s->lx = resp[7];                                 /* 左摇杆 X */
    s->ly = resp[8];                                 /* 左摇杆 Y */
    s->connected = 1;                                /* 标记在线 */
    return 1;                                        /* 本次数据有效 */
    }
