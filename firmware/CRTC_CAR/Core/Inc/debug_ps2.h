/*
 * debug_ps2.h
 *
 *  Created on: 2026年10月19日
 *      Author: zc
 *
 *  PS2 手柄独立调试程序。
 *  接线：CS=PB12，CLK=PB13，DAT=PB14，CMD=PB15（GPIO 软件模拟 SPI）
 */

#ifndef INC_DEBUG_PS2_H_
#define INC_DEBUG_PS2_H_

void Debug_PS2_Run(void);

#endif /* INC_DEBUG_PS2_H_ */
