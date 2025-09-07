#include "xhc_display_ui.h"
#include "xhc_receive.h"
#include "rotary_switch.h"
#include "ST7735.h"
#include "GFX_FUNCTIONS.h"
#include "XHC_DataStructures.h"
#include <stdio.h>


static uint8_t ui_initialized = 0;

void xhc_ui_init(void)
{
    fillScreen(WHITE);
    ST7735_FillRectangle(0, 95, DISPLAY_WIDTH, 33, BLUE);

    if (ui_initialized) return;

    /* Gesamtes Display weiß machen */


    /* WC-Bereich statische Labels - SCHWARZ auf WEISS */
    ST7735_WriteString(2, 2, "WC", Font_11x18, BLACK, WHITE);
    ST7735_WriteString(45, 2, "X:", Font_7x10, BLACK, WHITE);
    ST7735_WriteString(45, 17, "Y:", Font_7x10, BLACK, WHITE);
    ST7735_WriteString(45, 32, "Z:", Font_7x10, BLACK, WHITE);

    drawFastHLine(0, 47, 160, BLUE);

    /* MC-Bereich statische Labels - SCHWARZ auf WEISS */
    ST7735_WriteString(2, 51, "MC", Font_11x18, BLACK, WHITE);
    ST7735_WriteString(45, 51, "X:", Font_7x10, BLACK, WHITE);
    ST7735_WriteString(45, 66, "Y:", Font_7x10, BLACK, WHITE);
    ST7735_WriteString(45, 81, "Z:", Font_7x10, BLACK, WHITE);

    drawFastHLine(0, 96, 160, BLACK);

    /* Status-Balken Labels - WEISS auf BLAU */
    ST7735_WriteString(5, 98, "POS:", Font_7x10, WHITE, BLUE);
    ST7735_WriteString(85, 98, "STEP:", Font_7x10, WHITE, BLUE);

    ui_initialized = 1;
}

void xhc_ui_update_coordinates(void)
{
    char text[20];

    /* WC-Koordinaten mit Vorzeichen-Behandlung */
    // X-Koordinate
    uint16_t x_frac = output_report.pos[0].p_frac;
    uint8_t x_negative = (x_frac & 0x8000) ? 1 : 0;  // MSB = Vorzeichen
    x_frac &= 0x7FFF;  // Vorzeichen-Bit entfernen

    sprintf(text, "%5d.%04d", x_negative ? -(int)output_report.pos[0].p_int : (int)output_report.pos[0].p_int, x_frac);
    ST7735_WriteString(60, 2, text, Font_7x10, BLACK, WHITE);

    // Y-Koordinate
    uint16_t y_frac = output_report.pos[1].p_frac;
    uint8_t y_negative = (y_frac & 0x8000) ? 1 : 0;
    y_frac &= 0x7FFF;

    sprintf(text, "%5d.%04d", y_negative ? -(int)output_report.pos[1].p_int : (int)output_report.pos[1].p_int, y_frac);
    ST7735_WriteString(60, 17, text, Font_7x10, BLACK, WHITE);

    // Z-Koordinate
    uint16_t z_frac = output_report.pos[2].p_frac;
    uint8_t z_negative = (z_frac & 0x8000) ? 1 : 0;
    z_frac &= 0x7FFF;

    sprintf(text, "%5d.%04d", z_negative ? -(int)output_report.pos[2].p_int : (int)output_report.pos[2].p_int, z_frac);
    ST7735_WriteString(60, 32, text, Font_7x10, BLACK, WHITE);

    /* MC-Koordinaten (gleiche Logik) */
    // MC X
    uint16_t mx_frac = output_report.pos[3].p_frac;
    uint8_t mx_negative = (mx_frac & 0x8000) ? 1 : 0;
    mx_frac &= 0x7FFF;

    sprintf(text, "%5d.%04d", mx_negative ? -(int)output_report.pos[3].p_int : (int)output_report.pos[3].p_int, mx_frac);
    ST7735_WriteString(60, 50, text, Font_7x10, BLACK, WHITE);

    // MC Y
    uint16_t my_frac = output_report.pos[4].p_frac;
    uint8_t my_negative = (my_frac & 0x8000) ? 1 : 0;
    my_frac &= 0x7FFF;

    sprintf(text, "%5d.%04d", my_negative ? -(int)output_report.pos[4].p_int : (int)output_report.pos[4].p_int, my_frac);
    ST7735_WriteString(60, 65, text, Font_7x10, BLACK, WHITE);

    // MC Z
    uint16_t mz_frac = output_report.pos[5].p_frac;
    uint8_t mz_negative = (mz_frac & 0x8000) ? 1 : 0;
    mz_frac &= 0x7FFF;

    sprintf(text, "%5d.%04d", mz_negative ? -(int)output_report.pos[5].p_int : (int)output_report.pos[5].p_int, mz_frac);
    ST7735_WriteString(60, 80, text, Font_7x10, BLACK, WHITE);


    /* === DEBUG: Host→Device Geschwindigkeits-Daten === */

    /* Feedrate */
    sprintf(text, "F:%d", output_report.feedrate);
    ST7735_WriteString(35, 112, text, Font_7x10, BLACK, WHITE);

    /* Feedrate Override */
    sprintf(text, "FOv:%d", output_report.feedrate_ovr);
    ST7735_WriteString(80, 112, text, Font_7x10, BLACK, WHITE);

    /* Spindle Speed */
    //sprintf(text, "S:%d", output_report.sspeed);
    //ST7735_WriteString(80, 110, text, Font_7x10, BLACK, WHITE);

    /* Spindle Override */
    //sprintf(text, "SOv:%d", output_report.sspeed_ovr);
    //ST7735_WriteString(120, 110, text, Font_7x10, BLACK, WHITE);
}

void xhc_ui_update_status_bar(uint8_t rotary_pos, uint8_t step_mul)
{
    char text[10];

    /* Rotary Position (bleibt gleich) */
    const char* pos_text = "OFF";
    switch(rotary_pos) {
        case ROTARY_X: pos_text = "X"; break;
        case ROTARY_Y: pos_text = "Y"; break;
        case ROTARY_Z: pos_text = "Z"; break;
        case ROTARY_FEED: pos_text = "F"; break;
        case ROTARY_SPINDLE: pos_text = "S"; break;
        case ROTARY_A: pos_text = "A"; break;
    }

    sprintf(text, "%s    ", pos_text);
    ST7735_WriteString(35, 98, text, Font_7x10, WHITE, BLUE);

    /* Step als Integer-Werte */
    uint8_t low_nibble = step_mul & 0x0F;
    const char* step_text = "0.001";
    switch(low_nibble) {
        case 0x01: step_text = "0.001"; break;
        case 0x02: step_text = "0.005"; break;
        case 0x03: step_text = "0.010"; break;
        case 0x04: step_text = "0.020"; break;
        case 0x05: step_text = "0.030"; break;
        case 0x06: step_text = "0.040"; break;
        case 0x07: step_text = "0.050"; break;
        case 0x08: step_text = "0.100"; break;
        case 0x09: step_text = "0.500"; break;
        case 0x0A: step_text = "1.000"; break;
    }

    sprintf(text, "%s  ", step_text);
    ST7735_WriteString(120, 98, text, Font_7x10, WHITE, BLUE);


}

