/*
 * ps2.h
 *
 *  Created on: 2026年10月15日
 *      Author: zc
 */

#ifndef INC_PS2_H_
#define INC_PS2_H_

#include "main.h"
#include <stdint.h>

#define PS2_BTN_SELECT   0x0001   /* SELECT 键        —— 字节1 bit0 */
#define PS2_BTN_L3       0x0002   /* 左摇杆按下 L3    —— 字节1 bit1 */
#define PS2_BTN_R3       0x0004   /* 右摇杆按下 R3    —— 字节1 bit2 */
#define PS2_BTN_START    0x0008   /* START 键         —— 字节1 bit3 */
#define PS2_BTN_UP       0x0010   /* 十字键 上        —— 字节1 bit4 */
#define PS2_BTN_RIGHT    0x0020   /* 十字键 右        —— 字节1 bit5 */
#define PS2_BTN_DOWN     0x0040   /* 十字键 下        —— 字节1 bit6 */
#define PS2_BTN_LEFT     0x0080   /* 十字键 左        —— 字节1 bit7 */
#define PS2_BTN_L2       0x0100   /* 左肩键 L2        —— 字节2 bit0 */
#define PS2_BTN_R2       0x0200   /* 右肩键 R2        —— 字节2 bit1 */
#define PS2_BTN_L1       0x0400   /* 左肩键 L1        —— 字节2 bit2 */
#define PS2_BTN_R1       0x0800   /* 右肩键 R1        —— 字节2 bit3 */
#define PS2_BTN_TRIANGLE 0x1000   /* △ 三角           —— 字节2 bit4 */
#define PS2_BTN_CIRCLE   0x2000   /* ○ 圆圈           —— 字节2 bit5 */
#define PS2_BTN_CROSS    0x4000   /* ✕ 叉叉           —— 字节2 bit6 */
#define PS2_BTN_SQUARE   0x8000   /* □ 方块           —— 字节2 bit7 */

typedef struct{
    uint16_t buttons;    /* 按键位图：某位=1 → 该键正被按下（见上面掩码） */
    uint8_t  rx;         /* 右摇杆 X：0=最左   128≈中位   255=最右 */
    uint8_t  ry;         /* 右摇杆 Y：0=最上   128≈中位   255=最下 */
    uint8_t  lx;         /* 左摇杆 X：0=最左   128≈中位   255=最右 */
    uint8_t  ly;         /* 左摇杆 Y：0=最上   128≈中位   255=最下 */
    uint8_t  connected;  /* 1=本次读取有效；0=手柄掉线/未对频 */
} PS2_State_t;

void PS2_Init(void);
uint8_t PS2_Poll(PS2_State_t *state);

#endif /* INC_PS2_H_ */
