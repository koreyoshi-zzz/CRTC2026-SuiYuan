/*
 * motor.h
 *
 *  Created on: 2026年10月15日
 *      Author: zc
 */

#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "main.h"

#define MOTOR_FL 0//左前
#define MOTOR_RF 1//右前
#define MOTOR_RL 2//左后
#define MOTOR_RR 3//右后
#define MOTOR_NUM 4//电机总数

void Motor_Init(void);
/* 单电机调速：motor 取上面 4 个编号；speed -1000~+1000（负=反转，0=滑行） */
void Motor_Set(uint8_t motor,int16_t speed);
/* 整车运动：forward 前后(正=前进)、strafe 平移(正=向右)、rotate 旋转(正=顺时针)
 * 三个分量都是 -1000~+1000，可任意叠加（前进+右移 = 斜着走） */
void Motor_Drive(int16_t forward,int16_t strafe,int16_t rotate);

#endif /* INC_MOTOR_H_ */
