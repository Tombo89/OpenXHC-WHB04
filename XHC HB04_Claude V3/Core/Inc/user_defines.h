/*
 * user_defines.h
 *
 *  Created on: Sep 17, 2025
 *      Author: Thomas Weckmann
 */

#ifndef INC_USER_DEFINES_H_
#define INC_USER_DEFINES_H_

// Spindel Override (%) – typische Bereiche
#define SPINDLE_PERCENT_MIN   0    // z.B. 50 %
#define SPINDLE_PERCENT_MAX   150   // z.B. 200 %

// Feed Override (%) – typische Bereiche
#define FEED_PERCENT_MIN      0     // z.B. 0 %
#define FEED_PERCENT_MAX      150   // z.B. 150 %

/* =========================
   Umrechnungs-Teiler
   ========================= */
// je nach Software liefert output_report Werte in 1/100 %, 1/10 %, etc. Never use 0. If you need to turn it off for example for beamicon2 use 1
#define FEED_PERCENT_DIVISOR     1 //Bei Mach4 z.b 100
#define SPINDLE_PERCENT_DIVISOR  1


/* =========================
   XHC Button Codes
   ========================= */
#define BTN_Reset        0x17
#define BTN_Stop         0x16
#define BTN_Goto0        0x01
#define BTN_SS           0x02    // Start/Stop
#define BTN_Rewind       0x03
#define BTN_ProbeZ       0x04
#define BTN_Spindle      0x0C
#define BTN_Half         0x06    // 1/2
#define BTN_Zero         0x07    // 0
#define BTN_SafeZ        0x08
#define BTN_GotoHome     0x09
#define BTN_Macro1       0x0A
#define BTN_Macro2       0x0B
#define BTN_Macro3       0x05
#define BTN_Macro6       0x0F
#define BTN_Macro7       0x10
#define BTN_Step         0x0D    // Step+
#define BTN_MPG          0x0E


#endif /* INC_USER_DEFINES_H_ */
