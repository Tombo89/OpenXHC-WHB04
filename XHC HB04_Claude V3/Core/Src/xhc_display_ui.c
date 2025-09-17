#include "xhc_display_ui.h"
#include "xhc_receive.h"
#include "rotary_switch.h"
#include "st7735_dma.h"
#include "XHC_DataStructures.h"
#include <stdio.h>
#include "user_defines.h"



static uint8_t ui_initialized = 0;
static uint8_t lastposition = 0;


// kleine Helfer
static inline void HLine(int x, int y, int w, uint16_t c){
    for (int i=0;i<w;i++) ST7735_DrawPixel(x+i, y, c);
}
static inline void VLine(int x, int y, int h, uint16_t c){
    for (int i=0;i<h;i++) ST7735_DrawPixel(x, y+i, c);
}

void xhc_ui_init(void)
{

	ST7735_FillScreen(ST7735_WHITE);
    ST7735_FillRectangle(0, 94, DISPLAY_WIDTH, 34, ST7735_BLUE);


    if (ui_initialized) return;

    /* Gesamtes Display weiß machen */
   // ST7735_FillRectangleFast(50, 15, 108, Font_9x12.height + 2, ST7735_BLUE,);

    /* WC-Bereich statische Labels - SCHWARZ auf WEISS */
    ST7735_WriteString(2, 2, "WC", Font_11x18, ST7735_BLACK, ST7735_WHITE);
    ST7735_WriteString(38, 2, "X:", Font_9x11, ST7735_BLACK, ST7735_WHITE);
    ST7735_WriteString(38, 17, "Y:", Font_9x11, ST7735_BLACK, ST7735_WHITE);
    ST7735_WriteString(38, 32, "Z:", Font_9x11, ST7735_BLACK, ST7735_WHITE);



    //ST7735_WriteString(50, 2, " 1234567890.: XYZ", Font_9x11, ST7735_BLACK, ST7735_WHITE);
   // ST7735_WriteString(50, 17, "1.0000", Font_9x12, ST7735_BLACK, ST7735_WHITE);
    //ST7735_WriteString(50, 32, "-1000.0000", Font_9x12, ST7735_BLACK, ST7735_WHITE);

    HLine(2, 46, 156, ST7735_BLACK);

    /* MC-Bereich statische Labels - SCHWARZ auf WEISS */
    ST7735_WriteString(2, 49, "MC", Font_11x18, ST7735_BLACK, ST7735_WHITE);
    ST7735_WriteString(38, 49, "X:", Font_9x11, ST7735_BLACK, ST7735_WHITE);
    ST7735_WriteString(38, 64, "Y:", Font_9x11, ST7735_BLACK, ST7735_WHITE);
    ST7735_WriteString(38, 79, "Z:", Font_9x11, ST7735_BLACK, ST7735_WHITE);

    //ST7735_WriteString_GFX(65, 49, "no connection", &dosis_bold8pt7b, BLACK, WHITE);
    //ST7735_WriteString_GFX(65, 64, "no connection", &dosis_bold8pt7b, BLACK, WHITE);
    //ST7735_WriteString_GFX(65, 79, "no connection", &dosis_bold8pt7b, BLACK, WHITE);

    HLine(0, 93, 160, ST7735_BLACK);

    ST7735_WriteString(2, 95, "S:240000", Font_7x10, ST7735_WHITE, ST7735_BLUE);
    ST7735_WriteString(2, 106, "F:3500", Font_7x10, ST7735_WHITE, ST7735_BLUE);

    VLine(60,94,22,ST7735_BLACK);

    HLine(0, 115, 160, ST7735_BLACK);

    /* Labels für Rotary Pos und Step Mul. */
   ST7735_WriteString(2, 118, "POS:", Font_7x10, ST7735_WHITE, ST7735_BLUE);
   ST7735_WriteString(92, 118, "STP:", Font_7x10, ST7735_WHITE, ST7735_BLUE);



    //ST7735_WriteString(30, 98, "NC", Font_7x10, WHITE, BLUE);
    //ST7735_WriteString(30, 115, "NC", Font_7x10, WHITE, BLUE);

    //drawFastVLine(70, 94, 34, BLACK);

    /* Labels für feedrate und spindle speed */
    //ST7735_WriteString(75, 98, "S:", Font_7x10, WHITE, BLUE);
    //ST7735_WriteString(75, 115, "F:", Font_7x10, WHITE, BLUE);

    //ST7735_WriteString(90, 98, "NC", Font_7x10, WHITE, BLUE);
    //ST7735_WriteString(90, 115, "NC", Font_7x10, WHITE, BLUE);

    //ST7735_WriteString(130, 98, "100%", Font_7x10, WHITE, BLUE);
    //ST7735_WriteString(130, 115, "100%", Font_7x10, WHITE, BLUE);
    //ST7735_DrawRect(60, 2, 98, 13, ST7735_BLUE,1); // WC X
    //ST7735_DrawRect(60, 17, 98, 13, ST7735_BLUE,1); // WC Y
    //ST7735_DrawRect(60, 32, 98, 13, ST7735_BLUE,1); // WC Z
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
    sprintf(temp, "%5d.%04d", signed_value, frac);

    // Kopiere in den Ausgabepuffer (12 Zeichen + Nullterminator)
    sprintf(text, "%10s", temp);
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
        ST7735_WriteString(50, 2, text, Font_9x11, ST7735_BLACK, ST7735_WHITE);
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
        ST7735_WriteString(50, 17, text, Font_9x11, ST7735_BLACK, ST7735_WHITE);
        last_wc_y_int = wc_y_int;
        last_wc_y_frac = y_frac;
    }

    // WC Z - nur update wenn geändert
    uint16_t z_frac = output_report.pos[2].p_frac;
    uint8_t z_negative = (z_frac & 0x8000) ? 1 : 0;
    z_frac &= 0x7FFF;
    int32_t wc_z_int = z_negative ? -(int32_t)output_report.pos[2].p_int : (int32_t)output_report.pos[2].p_int;

    if (wc_z_int != last_wc_z_int || z_frac != last_wc_z_frac) {
        format_coordinate(text, (int)output_report.pos[2].p_int, z_frac, z_negative);
        ST7735_WriteString(50, 32, text, Font_9x11, ST7735_BLACK, ST7735_WHITE);
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
        format_coordinate(text, (int)output_report.pos[3].p_int, mx_frac, mx_negative);
        ST7735_WriteString(50, 49, text, Font_9x11, ST7735_BLACK, ST7735_WHITE);
        last_mc_x_int = mc_x_int;
        last_mc_x_frac = mx_frac;
    }

    // MC Y
    uint16_t my_frac = output_report.pos[4].p_frac;
    uint8_t my_negative = (my_frac & 0x8000) ? 1 : 0;
    my_frac &= 0x7FFF;
    int32_t mc_y_int = my_negative ? -(int32_t)output_report.pos[4].p_int : (int32_t)output_report.pos[4].p_int;

    if (mc_y_int != last_mc_y_int || my_frac != last_mc_y_frac) {
    	format_coordinate(text, (int)output_report.pos[4].p_int, my_frac, my_negative);
        ST7735_WriteString(50, 64, text, Font_9x11, ST7735_BLACK, ST7735_WHITE);
        last_mc_y_int = mc_y_int;
        last_mc_y_frac = my_frac;
    }

    // MC Z
    uint16_t mz_frac = output_report.pos[5].p_frac;
    uint8_t mz_negative = (mz_frac & 0x8000) ? 1 : 0;
    mz_frac &= 0x7FFF;
    int32_t mc_z_int = mz_negative ? -(int32_t)output_report.pos[5].p_int : (int32_t)output_report.pos[5].p_int;

    if (mc_z_int != last_mc_z_int || mz_frac != last_mc_z_frac) {
    	format_coordinate(text, (int)output_report.pos[5].p_int, mz_frac, mz_negative);
        ST7735_WriteString(50, 79, text, Font_9x11, ST7735_BLACK, ST7735_WHITE);
        last_mc_z_int = mc_z_int;
        last_mc_z_frac = mz_frac;
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
    ST7735_WriteString(32, 118, text, Font_7x10, ST7735_WHITE, ST7735_BLUE);

    if (lastposition != rotary_pos) {
        switch(lastposition) {



            case ROTARY_X: ST7735_WriteString(38, 2, "X:", Font_9x11, ST7735_BLACK, ST7735_WHITE); ST7735_WriteString(38, 49, "X:", Font_9x11, ST7735_BLACK, ST7735_WHITE);; break;
            case ROTARY_Y: ST7735_WriteString(38, 17, "Y:", Font_9x11, ST7735_BLACK, ST7735_WHITE); ST7735_WriteString(38, 64, "Y:", Font_9x11, ST7735_BLACK, ST7735_WHITE);; break;
            case ROTARY_Z: ST7735_WriteString(38, 32, "Z:", Font_9x11, ST7735_BLACK, ST7735_WHITE); ST7735_WriteString(38, 79, "Z:", Font_9x11, ST7735_BLACK, ST7735_WHITE);; break;
            //case ROTARY_FEED: ; break;
            //case ROTARY_SPINDLE: ; break;
            //case ROTARY_A:  ; break;
        }
        switch(rotary_pos) {
        case ROTARY_X: ST7735_WriteString(38, 2, "X:", Font_9x11, ST7735_RED, ST7735_WHITE); ST7735_WriteString(38, 49, "X:", Font_9x11, ST7735_RED, ST7735_WHITE);; break;
        case ROTARY_Y: ST7735_WriteString(38, 17, "Y:", Font_9x11, ST7735_RED, ST7735_WHITE); ST7735_WriteString(38, 64, "Y:", Font_9x11, ST7735_RED, ST7735_WHITE);; break;
        case ROTARY_Z: ST7735_WriteString(38, 32, "Z:", Font_9x11, ST7735_RED, ST7735_WHITE); ST7735_WriteString(38, 79, "Z:", Font_9x11, ST7735_RED, ST7735_WHITE);; break;
            //case ROTARY_FEED:  ; break;
            //case ROTARY_SPINDLE:  ; break;
            //case ROTARY_A:  ; break;
        }
        lastposition=rotary_pos;
    }

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
    ST7735_WriteString(122, 118, text, Font_7x10, ST7735_WHITE, ST7735_BLUE);



    // Spindel Statusbar
    uint16_t sspeed_percent = output_report.sspeed_ovr / SPINDLE_PERCENT_DIVISOR;
    ST7735_barProgressRange(95, 95, 60, 8, sspeed_percent, SPINDLE_PERCENT_MIN, SPINDLE_PERCENT_MAX,
                            ST7735_RED, ST7735_GREEN,
                            ST7735_WHITE, ST7735_BLACK,
                            1);
    //ST7735_barProgress(95, 95, 60, 8);

    sprintf(text, "%3u%%", sspeed_percent);
    ST7735_FillRectangle(63, 95, 31,7,ST7735_BLUE);
    ST7735_WriteString(63, 95, text, Font_7x10, ST7735_WHITE, ST7735_BLUE);





    // Feedrate Statusbar
    uint16_t feed_percent = output_report.feedrate_ovr / FEED_PERCENT_DIVISOR;
    ST7735_barProgressRange(95, 105, 60, 8, feed_percent, FEED_PERCENT_MIN, FEED_PERCENT_MAX,
                            ST7735_RED, ST7735_GREEN,
                            ST7735_WHITE, ST7735_BLACK,
                            1);
    //ST7735_barProgressRange(95, 105, 60, 8, feed_percent);
    //ST7735_barProgress(95, 95, 60, 8);

    sprintf(text, "%3u%%", feed_percent);
    ST7735_FillRectangle(63, 105, 31,7,ST7735_BLUE);
    ST7735_WriteString(63, 105, text, Font_7x10, ST7735_WHITE, ST7735_BLUE);



}
