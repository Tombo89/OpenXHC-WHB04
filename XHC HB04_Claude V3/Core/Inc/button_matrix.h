/*
 * Button Matrix Header für STM32CubeIDE
 */

#ifndef BUTTON_MATRIX_H
#define BUTTON_MATRIX_H

#include <stdint.h>

/* Funktionsprototypen */
void button_matrix_init(void);
uint8_t button_matrix_scan(uint8_t *btn1, uint8_t *btn2);
void button_matrix_display_test(void);

/* Button-Codes (aus XHC-Spezifikation) */
#define BTN_Reset       0x17
#define BTN_Stop        0x16
#define BTN_Goto0       0x01
#define BTN_SS          0x02    // Start/Stop
#define BTN_Rewind      0x03
#define BTN_ProbeZ      0x04
#define BTN_Spindle     0x0C
#define BTN_Half        0x06    // 1/2
#define BTN_Zero        0x07    // 0
#define BTN_SafeZ       0x08
#define BTN_GotoHome    0x09
#define BTN_Macro1      0x0A
#define BTN_Macro2      0x0B
#define BTN_Macro3      0x05
#define BTN_Macro6      0x0F
#define BTN_Macro7      0x10
#define BTN_Step        0x0D    // Step+
#define BTN_MPG         0x0E

#endif /* BUTTON_MATRIX_H */
