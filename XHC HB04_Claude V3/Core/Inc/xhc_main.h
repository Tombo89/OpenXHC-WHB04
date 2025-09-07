/*
 * XHC HB04 Main Header - Alle Funktionsprototypen
 */

#ifndef XHC_MAIN_H
#define XHC_MAIN_H

#include "XHC_DataStructures.h"
#include "xhc_receive.h"
#include "usbd_custom_hid_if.h"

/* Funktionsprototypen für main.c */
void xhc_custom_hid_init(void);
void xhc_main_loop(void);
uint8_t xhc_send_input_report(uint8_t btn1, uint8_t btn2, uint8_t wheel_mode, int8_t wheel_value);
void xhc_main_loop_encoder_only(void);

/* Debug-Funktionen */
void xhc_debug_callback(void);

#endif /* XHC_MAIN_H */
