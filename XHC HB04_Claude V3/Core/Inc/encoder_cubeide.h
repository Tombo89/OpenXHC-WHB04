/*
 * Encoder Header für STM32CubeIDE
 */

#ifndef ENCODER_CUBEIDE_H
#define ENCODER_CUBEIDE_H

#include <stdint.h>

/* Funktionsprototypen */
void encoder_init(void);
int16_t encoder_read(void);
void encoder_reset(void);
void encoder_display_test(void);

#endif /* ENCODER_CUBEIDE_H */
