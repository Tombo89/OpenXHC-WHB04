#ifndef ROTARY_SWITCH_H
#define ROTARY_SWITCH_H

#include <stdint.h>

/* Funktionsprototypen */
void rotary_switch_init(void);
uint8_t rotary_switch_read(void);
void rotary_switch_display_test(void);

/* Rotary Switch Werte */
#define ROTARY_OFF      0x00
#define ROTARY_X        0x11
#define ROTARY_Y        0x12
#define ROTARY_Z        0x13
#define ROTARY_FEED     0x14
#define ROTARY_SPINDLE  0x15
#define ROTARY_A        0x18

#endif /* ROTARY_SWITCH_H */
