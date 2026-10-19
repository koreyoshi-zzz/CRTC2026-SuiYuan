/*
 * motor.c
 *
 *  Created on: 2026年10月15日
 *      Author: zc
 *
 *  接线（按图）：每块 DRV8833 驱动两个电机，每个电机只用 2 根 PWM 线。
 *    M1 左前 FL = PA0/PA1  (TIM2_CH1 / TIM2_CH2)
 *    M2 右前 FR = PA2/PA3  (TIM2_CH3 / TIM2_CH4)
 *    M3 左后 RL = PA8/PA9  (TIM1_CH1 / TIM1_CH2)
 *    M4 右后 RR = PA10/PA11(TIM1_CH3 / TIM1_CH4)
 *
 *  DRV8833 的 2 线控制逻辑（IN1/IN2 直接决定电流方向）：
 *    IN1 = PWM, IN2 = 0   → 正转（PWM 低电平期间滑行）
 *    IN1 = 0,   IN2 = PWM → 反转
 *    IN1 = 0,   IN2 = 0   → 滑行停车
 *    IN1 = 1,   IN2 = 1   → 刹车（本项目没用）
 */
#include "motor.h"
#include "tim.h"

/* 一个电机的全部“资源”：挂在哪个定时器上、用哪两个通道 */
typedef struct{
    TIM_HandleTypeDef *htim;   /* 定时器句柄的指针（TIM1 或 TIM2） */
    uint32_t ch1;              /* IN1 对应的 PWM 通道 */
    uint32_t ch2;              /* IN2 对应的 PWM 通道 */
} Motor_t;


static const Motor_t motors[MOTOR_NUM] = {
    /* MOTOR_FL 左前 */ { &htim2, TIM_CHANNEL_1, TIM_CHANNEL_2 },   /* PA0 / PA1  */
    /* MOTOR_RF 右前 */ { &htim2, TIM_CHANNEL_3, TIM_CHANNEL_4 },   /* PA2 / PA3  */
    /* MOTOR_RL 左后 */ { &htim1, TIM_CHANNEL_1, TIM_CHANNEL_2 },   /* PA8 / PA9  */
    /* MOTOR_RR 右后 */ { &htim1, TIM_CHANNEL_3, TIM_CHANNEL_4 },   /* PA10/ PA11 */
};

#define MOTOR_SPEED_MAX 1000   /* speed 参数的满量程 */
#define MOTOR_PWM_FULL  3600   /* 定时器 ARR=3599，占空比满值 */
#define MOTOR_MAX_SPEED 800    /* 软件限速：保护 TT 电机，也给调试留余地 */

void Motor_Init(void){
    /* 上电先保证两路都为 0（滑行），再开 PWM 输出 */
    for (int i = 0; i < MOTOR_NUM; i++){
        __HAL_TIM_SET_COMPARE(motors[i].htim, motors[i].ch1, 0);
        __HAL_TIM_SET_COMPARE(motors[i].htim, motors[i].ch2, 0);
        HAL_TIM_PWM_Start(motors[i].htim, motors[i].ch1);
        HAL_TIM_PWM_Start(motors[i].htim, motors[i].ch2);
    }
}

/* 单电机调速：motor 取 MOTOR_FL/RF/RL/RR；speed -1000~+1000（负=反转，0=滑行） */
void Motor_Set(uint8_t motor, int16_t speed){
    if (motor >= MOTOR_NUM) return;
    if (speed >  MOTOR_MAX_SPEED) speed =  MOTOR_MAX_SPEED;
    if (speed < -MOTOR_MAX_SPEED) speed = -MOTOR_MAX_SPEED;

    /* 绝对值映射到 0~3600 的占空比 */
    uint32_t duty = (uint32_t)((speed >= 0) ? speed : -(int32_t)speed)
                    * MOTOR_PWM_FULL / MOTOR_SPEED_MAX;

    if (speed > 0){          /* 正转：IN1=PWM, IN2=0 */
        __HAL_TIM_SET_COMPARE(motors[motor].htim, motors[motor].ch1, duty);
        __HAL_TIM_SET_COMPARE(motors[motor].htim, motors[motor].ch2, 0);
    } else if (speed < 0){   /* 反转：IN1=0, IN2=PWM */
        __HAL_TIM_SET_COMPARE(motors[motor].htim, motors[motor].ch1, 0);
        __HAL_TIM_SET_COMPARE(motors[motor].htim, motors[motor].ch2, duty);
    } else {                 /* 滑行：两路都 0 */
        __HAL_TIM_SET_COMPARE(motors[motor].htim, motors[motor].ch1, 0);
        __HAL_TIM_SET_COMPARE(motors[motor].htim, motors[motor].ch2, 0);
    }
}

/* 整车运动：forward 前后(正=前进)、strafe 平移(正=向右)、rotate 旋转(正=顺时针)
 * 三个分量都是 -1000~+1000，可任意叠加（前进+右移 = 斜着走） */
void Motor_Drive(int16_t forward, int16_t strafe, int16_t rotate){
    /* 麦克纳姆轮逆运动学：三个运动分量 → 四个轮子各自的速度 */
    int32_t v[MOTOR_NUM];
    v[MOTOR_FL] = (int32_t)forward + strafe + rotate;   /* 左前 */
    v[MOTOR_RF] = (int32_t)forward - strafe - rotate;   /* 右前 */
    v[MOTOR_RL] = (int32_t)forward - strafe + rotate;   /* 左后 */
    v[MOTOR_RR] = (int32_t)forward + strafe - rotate;   /* 右后 */

    /* 归一化：叠加后可能超过 ±1000，整体等比缩小，保住运动方向 */
    int32_t maxabs = 1;                     /* 从 1 起，防除零 */
    for (int i = 0; i < MOTOR_NUM; i++){
        int32_t a = (v[i] >= 0) ? v[i] : -v[i];
        if (a > maxabs) maxabs = a;
    }
    for (int i = 0; i < MOTOR_NUM; i++){
        int32_t out = (maxabs > MOTOR_SPEED_MAX)
                    ? (v[i] * MOTOR_SPEED_MAX / maxabs) : v[i];
        Motor_Set((uint8_t)i, (int16_t)out);
    }
}
