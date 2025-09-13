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
    ST7735_WriteString_GFX(2, 2, "WC", &FreeSansBold9pt7b, BLACK, WHITE);
    ST7735_WriteString_GFX(45, 2, "X:", &dosis_bold8pt7b, BLACK, WHITE);
    ST7735_WriteString_GFX(45, 17, "Y:", &dosis_bold8pt7b, BLACK, WHITE);
    ST7735_WriteString_GFX(45, 32, "Z:", &dosis_bold8pt7b, BLACK, WHITE);



    //ST7735_WriteString_GFX(65, 2, "no connection", &dosis_bold8pt7b, BLACK, WHITE);
    //ST7735_WriteString_GFX(65, 17, "no connection", &dosis_bold8pt7b, BLACK, WHITE);
    //ST7735_WriteString_GFX(65, 32, "no connection", &dosis_bold8pt7b, BLACK, WHITE);

    drawFastHLine(0, 47, 160, BLUE);

    /* MC-Bereich statische Labels - SCHWARZ auf WEISS */
    ST7735_WriteString_GFX(2, 49, "MC", &FreeSansBold9pt7b, BLACK, WHITE);
    ST7735_WriteString_GFX(45, 49, "X:", &dosis_bold8pt7b, BLACK, WHITE);
    ST7735_WriteString_GFX(45, 64, "Y:", &dosis_bold8pt7b, BLACK, WHITE);
    ST7735_WriteString_GFX(45, 79, "Z:", &dosis_bold8pt7b, BLACK, WHITE);

    //ST7735_WriteString_GFX(65, 49, "no connection", &dosis_bold8pt7b, BLACK, WHITE);
    //ST7735_WriteString_GFX(65, 64, "no connection", &dosis_bold8pt7b, BLACK, WHITE);
    //ST7735_WriteString_GFX(65, 79, "no connection", &dosis_bold8pt7b, BLACK, WHITE);

    drawFastHLine(0, 94, 160, BLACK);

    /* Labels für Rotary Pos und Step Mul. */
    ST7735_WriteString(2, 98, "POS:", Font_7x10, WHITE, BLUE);
    ST7735_WriteString(2, 115, "STP:", Font_7x10, WHITE, BLUE);

    ST7735_WriteString(30, 98, "NC", Font_7x10, WHITE, BLUE);
    ST7735_WriteString(30, 115, "NC", Font_7x10, WHITE, BLUE);

    drawFastVLine(70, 94, 34, BLACK);

    /* Labels für feedrate und spindle speed */
    ST7735_WriteString(75, 98, "S:", Font_7x10, WHITE, BLUE);
    ST7735_WriteString(75, 115, "F:", Font_7x10, WHITE, BLUE);

    ST7735_WriteString(90, 98, "NC", Font_7x10, WHITE, BLUE);
    ST7735_WriteString(90, 115, "NC", Font_7x10, WHITE, BLUE);

    ST7735_WriteString(130, 98, "100%", Font_7x10, WHITE, BLUE);
    ST7735_WriteString(130, 115, "100%", Font_7x10, WHITE, BLUE);

    ui_initialized = 1;
}


/**
 * @brief Formatiert Koordinate mit rechtsbündiger Ausrichtung und fixen Dezimalpunkten
 * @param text Output-String (muss mindestens 13 Zeichen haben)
 * @param value Integer-Teil der Koordinate
 * @param frac Fraktionaler Teil (16-bit für WHB04)
 * @param negative Vorzeichen-Flag
 *
 * Format: "   -123.4567" oder "      0.0000" (rechtsbündig, 12 Zeichen)
 * Dezimalpunkt ist immer an Position 8 (von links)
 */
void format_coordinate(char* text, int value, uint16_t frac, uint8_t negative)
{
    char temp[16];
    int signed_value = negative ? -value : value;

    // Formatiere die Zahl mit führenden Leerzeichen für rechtsbündige Ausrichtung
    // Format: "ssss-nnn.ffff" wobei s=Leerzeichen, n=Ziffern, f=Nachkommastellen
    sprintf(temp, "%7d.%04d", signed_value, frac);

    // Kopiere in den Ausgabepuffer (12 Zeichen + Nullterminator)
    sprintf(text, "%12s", temp);
}

void xhc_ui_update_coordinates(void)
{
    char text[20];

    // Statische Variablen für Cache
    static int32_t last_wc_x_int = -999999, last_wc_y_int = -999999, last_wc_z_int = -999999;
    static int32_t last_mc_x_int = -999999, last_mc_y_int = -999999, last_mc_z_int = -999999;
    static uint16_t last_wc_x_frac = 0xFFFF, last_wc_y_frac = 0xFFFF, last_wc_z_frac = 0xFFFF;
    static uint16_t last_mc_x_frac = 0xFFFF, last_mc_y_frac = 0xFFFF, last_mc_z_frac = 0xFFFF;

    // WC X - nur update wenn geändert
    uint16_t x_frac = output_report.pos[0].p_frac;
    uint8_t x_negative = (x_frac & 0x8000) ? 1 : 0;
    x_frac &= 0x7FFF;
    int32_t wc_x_int = x_negative ? -(int32_t)output_report.pos[0].p_int : (int32_t)output_report.pos[0].p_int;

    if (wc_x_int != last_wc_x_int || x_frac != last_wc_x_frac) {
        format_coordinate(text, (int)output_report.pos[0].p_int, x_frac, x_negative);
        ST7735_WriteString_GFX(65, 2, text, &dosis_bold8pt7b, BLACK, WHITE);
        last_wc_x_int = wc_x_int;
        last_wc_x_frac = x_frac;
    }

    // WC Y - nur update wenn geändert
    uint16_t y_frac = output_report.pos[1].p_frac;
    uint8_t y_negative = (y_frac & 0x8000) ? 1 : 0;
    y_frac &= 0x7FFF;
    int32_t wc_y_int = y_negative ? -(int32_t)output_report.pos[1].p_int : (int32_t)output_report.pos[1].p_int;

    if (wc_y_int != last_wc_y_int || y_frac != last_wc_y_frac) {
        format_coordinate(text, (int)output_report.pos[1].p_int, y_frac, y_negative);
        ST7735_WriteString_GFX(65, 17, text, &dosis_bold8pt7b, BLACK, WHITE);
        last_wc_y_int = wc_y_int;
        last_wc_y_frac = y_frac;
    }

    // WC Z - nur update wenn geändert
    uint16_t z_frac = output_report.pos[2].p_frac;
    uint8_t z_negative = (z_frac & 0x8000) ? 1 : 0;
    z_frac &= 0x7FFF;
    int32_t wc_z_int = z_negative ? -(int32_t)output_report.pos[2].p_int : (int32_t)output_report.pos[2].p_int;

    if (wc_z_int != last_wc_z_int || z_frac != last_wc_z_frac) {
        sprintf(text, "%6d.%04d", z_negative ? -(int)output_report.pos[2].p_int : (int)output_report.pos[2].p_int, z_frac);
        ST7735_WriteString_GFX(65, 32, text, &dosis_bold8pt7b, BLACK, WHITE);
        last_wc_z_int = wc_z_int;
        last_wc_z_frac = z_frac;
    }

    // MC Koordinaten - gleiche Logik
    // MC X
    uint16_t mx_frac = output_report.pos[3].p_frac;
    uint8_t mx_negative = (mx_frac & 0x8000) ? 1 : 0;
    mx_frac &= 0x7FFF;
    int32_t mc_x_int = mx_negative ? -(int32_t)output_report.pos[3].p_int : (int32_t)output_report.pos[3].p_int;

    if (mc_x_int != last_mc_x_int || mx_frac != last_mc_x_frac) {
        sprintf(text, "%6d.%04d", mx_negative ? -(int)output_report.pos[3].p_int : (int)output_report.pos[3].p_int, mx_frac);
        ST7735_WriteString_GFX(65, 49, text, &dosis_bold8pt7b, BLACK, WHITE);
        last_mc_x_int = mc_x_int;
        last_mc_x_frac = mx_frac;
    }

    // MC Y
    uint16_t my_frac = output_report.pos[4].p_frac;
    uint8_t my_negative = (my_frac & 0x8000) ? 1 : 0;
    my_frac &= 0x7FFF;
    int32_t mc_y_int = my_negative ? -(int32_t)output_report.pos[4].p_int : (int32_t)output_report.pos[4].p_int;

    if (mc_y_int != last_mc_y_int || my_frac != last_mc_y_frac) {
        sprintf(text, "%6d.%04d", my_negative ? -(int)output_report.pos[4].p_int : (int)output_report.pos[4].p_int, my_frac);
        ST7735_WriteString_GFX(65, 64, text, &dosis_bold8pt7b, BLACK, WHITE);
        last_mc_y_int = mc_y_int;
        last_mc_y_frac = my_frac;
    }

    // MC Z
    uint16_t mz_frac = output_report.pos[5].p_frac;
    uint8_t mz_negative = (mz_frac & 0x8000) ? 1 : 0;
    mz_frac &= 0x7FFF;
    int32_t mc_z_int = mz_negative ? -(int32_t)output_report.pos[5].p_int : (int32_t)output_report.pos[5].p_int;

    if (mc_z_int != last_mc_z_int || mz_frac != last_mc_z_frac) {
        sprintf(text, "%6d.%04d", mz_negative ? -(int)output_report.pos[5].p_int : (int)output_report.pos[5].p_int, mz_frac);
        ST7735_WriteString_GFX(65, 79, text, &dosis_bold8pt7b, BLACK, WHITE);
        last_mc_z_int = mc_z_int;
        last_mc_z_frac = mz_frac;
    }

    // Feedrate/Spindle - nur einmal pro Sekunde oder bei Änderung
    static uint16_t last_feedrate = 0xFFFF, last_feedrate_ovr = 0xFFFF;
    static uint32_t last_feed_update = 0;
    uint32_t now = HAL_GetTick();

    if (output_report.feedrate != last_feedrate ||
        output_report.feedrate_ovr != last_feedrate_ovr ||
        (now - last_feed_update > 1000)) {

        sprintf(text, "%d", output_report.feedrate);
        ST7735_WriteString(90, 115, text, Font_7x10, WHITE, BLUE);

        sprintf(text, "%d", output_report.feedrate_ovr);
        ST7735_WriteString(130, 115, text, Font_7x10, WHITE, BLUE);

        last_feedrate = output_report.feedrate;
        last_feedrate_ovr = output_report.feedrate_ovr;
        last_feed_update = now;
    }
}

void xhc_ui_update_status_bar(uint8_t rotary_pos, uint8_t step_mul)
{
    char text[10];

    /* Rotary Position */
    const char* pos_text = "OFF";
    switch(rotary_pos) {
        case ROTARY_X: pos_text = "X"; break;
        case ROTARY_Y: pos_text = "Y"; break;
        case ROTARY_Z: pos_text = "Z"; break;
        case ROTARY_FEED: pos_text = "F"; break;
        case ROTARY_SPINDLE: pos_text = "S"; break;
        case ROTARY_A: pos_text = "A"; break;
    }

    sprintf(text, "%s  ", pos_text);
    ST7735_WriteString(30, 98, text, Font_7x10, WHITE, BLUE);

    /* Step Multiplier mit besserer Formatierung */
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

    sprintf(text, "%s", step_text);
    ST7735_WriteString(30, 115, text, Font_7x10, WHITE, BLUE);
}
