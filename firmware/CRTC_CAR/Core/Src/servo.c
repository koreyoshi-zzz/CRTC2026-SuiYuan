/*
 * servo.c
 *
 *  Created on: 2026年10月15日
 *      Author: zc
 *
 *  三个舵机都挂在 TIM4（50Hz 舵机专用频率）：
 *    舵机1 SG90  = PB6 (TIM4_CH1)
 *    舵机2 SG90  = PB7 (TIM4_CH2)
 *    舵机3 MG995 = PB8 (TIM4_CH3)
 *
 *  定时器 1 格 = 1us（72MHz / 72），ARR=20000 → 周期 20ms = 50Hz。
 *  舵机角度由脉冲宽度决定：0°≈500us，180°≈2500us。
 */
#include "main.h"
#include "servo.h"
#include "tim.h"

static const uint32_t servo_ch[3] = {
    TIM_CHANNEL_1,   /* SERVO_SG90_1 → PB6 */
    TIM_CHANNEL_2,   /* SERVO_SG90_2 → PB7 */
    TIM_CHANNEL_3,   /* SERVO_MG995  → PB8 */
};

#define PULSE_MIN_US 500     /* 0°   对应的脉宽 */
#define PULSE_MAX_US 2500    /* 180° 对应的脉宽 */

void Servo_Init(void){
    for (int i = 0; i < 3; i++){
        HAL_TIM_PWM_Start(&htim4, servo_ch[i]);
    }
    Servo_SetAngle(SERVO_SG90_1, 90.0f);   /* 上电全部回中 */
    Servo_SetAngle(SERVO_SG90_2, 90.0f);
    Servo_SetAngle(SERVO_MG995,  90.0f);
}

void Servo_SetAngle(uint8_t id, float angle){
    if (id > 2) return;
    if (angle < 0.0f)   angle = 0.0f;
    if (angle > 180.0f) angle = 180.0f;

    /* 角度线性映射成脉宽（us），1 格 = 1us，直接写进比较寄存器 */
    uint32_t pulse = PULSE_MIN_US
                   + (uint32_t)(angle * ((float)(PULSE_MAX_US - PULSE_MIN_US) / 180.0f));
    __HAL_TIM_SET_COMPARE(&htim4, servo_ch[id], pulse);
}
