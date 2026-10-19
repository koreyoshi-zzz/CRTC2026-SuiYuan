/*
 * debug_config.h
 *
 *  Created on: 2026年10月19日
 *      Author: zc
 *
 *  分模块调试用的总开关。
 *
 *  用法：把 DEBUG_MODE 改成下面四个值之一，重新编译下载即可。
 *  调试代码用 #if 包裹，选 0（正常模式）时完全不参与编译，不影响正式程序。
 */

#ifndef INC_DEBUG_CONFIG_H_
#define INC_DEBUG_CONFIG_H_

/* ---- 调试模式总开关：四选一 ---- */
#define DEBUG_MODE_NORMAL   0   /* 正常运行：手柄遥控整车（正式程序） */
#define DEBUG_MODE_SERVO    1   /* 只调试舵机 */
#define DEBUG_MODE_MOTOR    2   /* 只调试电机 */
#define DEBUG_MODE_PS2      3   /* 只调试 PS2 手柄 */

#ifndef DEBUG_MODE
#define DEBUG_MODE   DEBUG_MODE_NORMAL
#endif

/* ---- 各调试程序的函数声明 ---- */
#if DEBUG_MODE == DEBUG_MODE_SERVO
void Debug_Servo_Run(void);
#elif DEBUG_MODE == DEBUG_MODE_MOTOR
void Debug_Motor_Run(void);
#elif DEBUG_MODE == DEBUG_MODE_PS2
void Debug_PS2_Run(void);
#endif

#endif /* INC_DEBUG_CONFIG_H_ */
