/*
 * debug_servo.h
 *
 *  Created on: 2026年10月19日
 *      Author: zc
 *
 *  舵机独立调试程序。
 *  接线：舵机1 SG90 = PB6，舵机2 SG90 = PB7，舵机3 MG995 = PB8（全部 TIM4，50Hz）
 */

#ifndef INC_DEBUG_SERVO_H_
#define INC_DEBUG_SERVO_H_

void Debug_Servo_Run(void);

#endif /* INC_DEBUG_SERVO_H_ */
