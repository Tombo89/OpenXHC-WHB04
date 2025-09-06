/*
 * XHC HB04 Datenempfang Header für STM32CubeIDE
 */

#ifndef XHC_RECEIVE_H
#define XHC_RECEIVE_H

#include "XHC_DataStructures.h"

/* Achsendefinitionen */
#define XHC_AXIS_X  0
#define XHC_AXIS_Y  1
#define XHC_AXIS_Z  2
#define XHC_AXIS_A  3
#define XHC_AXIS_B  4
#define XHC_AXIS_C  5

/* Koordinatensystem */
#define XHC_WORKSPACE_COORDS    0
#define XHC_MACHINE_COORDS      1

/* Maschinenstatus-Bits */
#define XHC_STATE_RUN_BLINK     0x01
#define XHC_STATE_PAUSE_BLINK   0x02
#define XHC_STATE_XHB04_FLAG    0x20
#define XHC_STATE_FLASH_LEDS    0x40

/* Step-Multiplikator Definitionen */
#define XHC_STEP_MUL_0X1X       0x00
#define XHC_STEP_MUL_1X1X       0x01
#define XHC_STEP_MUL_5X1X       0x02
#define XHC_STEP_MUL_10X1X      0x03
#define XHC_STEP_MUL_20X1X      0x04
#define XHC_STEP_MUL_30X1X      0x05
#define XHC_STEP_MUL_40X1X      0x06
#define XHC_STEP_MUL_50X1X      0x07
#define XHC_STEP_MUL_100X1X     0x08
#define XHC_STEP_MUL_500X1X     0x09
#define XHC_STEP_MUL_1000X1X    0x0A

/* Hi-Nibble Definitionen */
#define XHC_STEP_ORIGIN         0x10
#define XHC_STEP_FLOATING       0x20
#define XHC_STEP_MECH_ORIGIN    0x50
#define XHC_STEP_FINE_ADJ       0x60

/* Hauptfunktionen */
void xhc_recv(uint8_t *data);
void xhc_process_received_data(void);

/* Hilfsfunktionen */
float xhc_get_position(uint8_t axis, uint8_t is_machine);
uint8_t xhc_get_machine_state(void);
uint8_t xhc_get_step_multiplier(void);

/* Inline-Hilfsfunktionen für häufig verwendete Werte */
static inline uint16_t xhc_get_feedrate(void) {
    return output_report.feedrate;
}

static inline uint16_t xhc_get_feedrate_override(void) {
    return output_report.feedrate_ovr;
}

static inline uint16_t xhc_get_spindle_speed(void) {
    return output_report.sspeed;
}

static inline uint16_t xhc_get_spindle_override(void) {
    return output_report.sspeed_ovr;
}

static inline uint8_t xhc_get_day(void) {
    return day;
}

/* Makros für Statusprüfung */
#define XHC_IS_RUNNING()    (output_report.state & XHC_STATE_RUN_BLINK)
#define XHC_IS_PAUSED()     (output_report.state & XHC_STATE_PAUSE_BLINK)
#define XHC_FLASH_LEDS()    (output_report.state & XHC_STATE_FLASH_LEDS)

#endif /* XHC_RECEIVE_H */
