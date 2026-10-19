/*
 * debug_motor.c
 *
 *  Created on: 2026年10月19日
 *      Author: zc
 *
 *  电机 / 底盘独立调试程序（配合 debug_config.h 的 DEBUG_MODE_MOTOR 使用）
 *
 *  ⚠️ 调试前务必把车架起来（四轮离地），否则车会直接冲出去。
 *     本程序用的速度是 DEBUG_SPEED（默认 400），比正式程序的 800 低一半，
 *     就是为了观察时更可控。
 *
 *  ── 怎么看结果 ────────────────────────────────────────────
 *  PC13 LED 进入每项前快闪 N 下（N = 测试项编号），然后开始动作。
 *
 *    闪 1 下 → 四个电机【逐个】正转 1 秒：FL→FR→RL→RR（核对接线、转向）
 *    闪 2 下 → 四个电机【逐个】反转 1 秒：FL→FR→RL→RR
 *    闪 3 下 → 四电机同时正转 2 秒（看总电流、电源是否被拉垮）
 *    闪 4 下 → 整车前进 2 秒 → 停 0.5 秒 → 后退 2 秒
 *    闪 5 下 → 整车右平移 2 秒 → 停 0.5 秒 → 左平移 2 秒
 *    闪 6 下 → 整车顺时针旋转 2 秒 → 停 0.5 秒 → 逆时针旋转 2 秒
 *    闪 7 下 → 斜向运动（前进+右移），验证麦轮运动合成
 *
 *  ── 常见故障对照 ──────────────────────────────────────────
 *  · 某电机不转          → 查该路 TIM 通道是否 Start、DRV8833 的 IN 线是否松动
 *  · 电机转向与预期相反  → 把 motor.c 里该电机 ch1/ch2 的定义对调，或本文件里取负号
 *  · 前进时车歪          → 左右轮实际转速不一致，检查轮胎是否打滑、电机减速比是否一致
 *  · 平移不动 / 原地打转 → 麦轮辊子方向装反了，把对角两个轮子的安装方向核对一遍
 *  · 一动就复位          → 电机启动电流大，电源功率不足或线径太细，加大电容 / 换粗线
 */
#include "debug_config.h"

#if DEBUG_MODE == DEBUG_MODE_MOTOR

#include "main.h"
#include "motor.h"
#include "tim.h"

#define LED_ON()     HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define LED_OFF()    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)

/* 调试用速度：满量程是 ±1000，正式程序跑 800，这里减半更可控 */
#define DEBUG_SPEED   400
#define DEBUG_DELAY   1000     /* 单电机测试时长（ms） */
#define MOVE_DELAY    2000     /* 整车动作时长（ms）  */
#define GAP_DELAY     500      /* 动作之间的停顿      */

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

/* 四个轮子全部停（滑行） */
static void stop_all(void)
{
    for (uint8_t i = 0; i < MOTOR_NUM; i++) {
        Motor_Set(i, 0);
    }
}

/* 四轮编号对照（方便读代码时核对接线）：
 *   MOTOR_FL(0) = 左前 = PA0/PA1
 *   MOTOR_RF(1) = 右前 = PA2/PA3
 *   MOTOR_RL(2) = 左后 = PA8/PA9
 *   MOTOR_RR(3) = 右后 = PA10/PA11 */

void Debug_Motor_Run(void)
{
    Motor_Init();
    LED_OFF();
    stop_all();
    HAL_Delay(800);

    while (1)
    {
        /* ---- 测试 1：四个电机逐个正转 1 秒 ---- */
        blink(1);
        for (uint8_t i = 0; i < MOTOR_NUM; i++) {
            stop_all();
            HAL_Delay(200);
            Motor_Set(i, DEBUG_SPEED);      /* 正转 */
            LED_ON();
            HAL_Delay(DEBUG_DELAY);
            LED_OFF();
        }
        stop_all();
        HAL_Delay(GAP_DELAY);

        /* ---- 测试 2：四个电机逐个反转 1 秒 ---- */
        blink(2);
        for (uint8_t i = 0; i < MOTOR_NUM; i++) {
            stop_all();
            HAL_Delay(200);
            Motor_Set(i, -DEBUG_SPEED);     /* 反转 */
            LED_ON();
            HAL_Delay(DEBUG_DELAY);
            LED_OFF();
        }
        stop_all();
        HAL_Delay(GAP_DELAY);

        /* ---- 测试 3：四电机同时正转 2 秒 ---- */
        blink(3);
        for (uint8_t i = 0; i < MOTOR_NUM; i++) {
            Motor_Set(i, DEBUG_SPEED);
        }
        LED_ON();
        HAL_Delay(2000);
        LED_OFF();
        stop_all();
        HAL_Delay(GAP_DELAY);

        /* ---- 测试 4：整车前进 → 后退 ---- */
        blink(4);
        Motor_Drive( DEBUG_SPEED, 0, 0);    /* 前进 */
        LED_ON();
        HAL_Delay(MOVE_DELAY);
        LED_OFF();
        stop_all();
        HAL_Delay(GAP_DELAY);

        Motor_Drive(-DEBUG_SPEED, 0, 0);    /* 后退 */
        LED_ON();
        HAL_Delay(MOVE_DELAY);
        LED_OFF();
        stop_all();
        HAL_Delay(GAP_DELAY);

        /* ---- 测试 5：右平移 → 左平移 ---- */
        blink(5);
        Motor_Drive(0,  DEBUG_SPEED, 0);    /* 右移 */
        LED_ON();
        HAL_Delay(MOVE_DELAY);
        LED_OFF();
        stop_all();
        HAL_Delay(GAP_DELAY);

        Motor_Drive(0, -DEBUG_SPEED, 0);    /* 左移 */
        LED_ON();
        HAL_Delay(MOVE_DELAY);
        LED_OFF();
        stop_all();
        HAL_Delay(GAP_DELAY);

        /* ---- 测试 6：顺时针旋转 → 逆时针旋转 ---- */
        blink(6);
        Motor_Drive(0, 0,  DEBUG_SPEED);    /* 顺时针 */
        LED_ON();
        HAL_Delay(MOVE_DELAY);
        LED_OFF();
        stop_all();
        HAL_Delay(GAP_DELAY);

        Motor_Drive(0, 0, -DEBUG_SPEED);    /* 逆时针 */
        LED_ON();
        HAL_Delay(MOVE_DELAY);
        LED_OFF();
        stop_all();
        HAL_Delay(GAP_DELAY);

        /* ---- 测试 7：斜向运动（前进 + 右移），验证麦轮合成 ---- */
        blink(7);
        Motor_Drive(DEBUG_SPEED / 2, DEBUG_SPEED / 2, 0);   /* 右前方斜行 */
        LED_ON();
        HAL_Delay(MOVE_DELAY);
        LED_OFF();
        stop_all();
        HAL_Delay(GAP_DELAY);

        /* 一轮跑完，停 2 秒后循环 */
        HAL_Delay(2000);
    }
}

#endif /* DEBUG_MODE == DEBUG_MODE_MOTOR */
