/*
 * servo.h
 *
 *  Created on: 2026年10月15日
 *      Author: zc
 */

#ifndef INC_SERVO_H_
#define INC_SERVO_H_

#include "main.h"

#define SERVO_SG90_1 0
#define SERVO_SG90_2 1
#define SERVO_MG995 2

void Servo_Init(void);
void Servo_SetAngle(uint8_t id, float angle);

#endif /* INC_SERVO_H_ */
