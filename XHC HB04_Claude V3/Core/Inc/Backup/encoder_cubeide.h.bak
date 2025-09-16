/*
 * Encoder Header für STM32CubeIDE
 */

#ifndef ENCODER_CUBEIDE_H
#define ENCODER_CUBEIDE_H

#include <stdint.h>

extern uint16_t enc_prev_cnt;
extern int32_t enc_rem;
extern volatile int16_t encoder_1ms_buffer;
extern volatile int32_t impulse_buffer;

/* Funktionsprototypen */
void encoder_init(void);
int16_t encoder_read(void);
uint8_t encoder_has_activity(void);
void encoder_reset(void);
void encoder_display_test(void);
void encoder_display_sent_values(void);
void encoder_1ms_poll(void);
int16_t encoder_read_1ms(void);
uint8_t encoder_1ms_has_data(void);
int16_t encoder_read_simple_speed(void);
void encoder_reset_buffers(void);

#endif /* ENCODER_CUBEIDE_H */
