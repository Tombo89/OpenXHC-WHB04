/*
 * XHC HB04 Main Implementation für STM32CubeIDE
 */

#include "xhc_main.h"
#include "main.h"
#include <string.h>
#include "ST7735.h"
#include "encoder_cubeide.h"
#include "button_matrix.h"
#include "rotary_switch.h"
#include "xhc_display_ui.h"
#include "GFX_FUNCTIONS.h"

/* Externe Variablen */
extern USBD_HandleTypeDef hUsbDeviceFS;

// Globale Zustandsvariablen
static uint8_t current_btn1 = 0;
static uint8_t current_btn2 = 0;
static uint8_t current_wheel_mode = 0x11;

// State Machine für non-blocking Updates
typedef enum {
    MAIN_STATE_ENCODER,
    MAIN_STATE_BUTTONS,
    MAIN_STATE_ROTARY,
    MAIN_STATE_USB_SEND,
    MAIN_STATE_DISPLAY_COORDS,
    MAIN_STATE_DISPLAY_STATUS,
    MAIN_STATE_COUNT
} main_state_t;

static main_state_t current_state = MAIN_STATE_ENCODER;
static uint32_t last_state_time = 0;

/**
 * @brief Initialisierung der XHC Custom HID Integration
 */
void xhc_custom_hid_init(void)
{
    /* Initialisiere XHC-spezifische Variablen */
    memset(&output_report, 0, sizeof(output_report));
    memset(&in_report, 0, sizeof(in_report));
    in_report.id = 0x04;  // Report ID für Device→Host
}

/**
 * @brief Sendet XHC Input Report (Report ID 0x04) zum Host
 */
uint8_t xhc_send_input_report(uint8_t btn1, uint8_t btn2, uint8_t wheel_mode, int8_t wheel_value)
{
    static uint32_t last_successful_send = 0;
    uint32_t current_time = HAL_GetTick();

    // Mindestens 5ms zwischen USB-Sends (wichtig!)
    if (current_time - last_successful_send < 5) {
        return USBD_BUSY;  // Zu früh für nächsten Send
    }

    /* Fülle die Input Report Struktur */
    in_report.btn_1 = btn1;
    in_report.btn_2 = btn2;
    in_report.wheel_mode = wheel_mode;
    in_report.wheel = wheel_value;
    in_report.xor_day = xhc_get_day() ^ btn1;

    /* Sende Report über Custom HID */
    uint8_t result = USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&in_report, sizeof(in_report));

    if (result == USBD_OK) {
        last_successful_send = current_time;
        return USBD_OK;
    } else {
    	return USBD_FAIL;
    }
}

/**
 * @brief XHC Hauptschleife für die Anwendung - mit 1ms Encoder-Abtastung
 */
void xhc_main_loop(void)
{
    static int32_t accumulator = 0;
    static uint32_t last_send = 0;
    static uint8_t current_btn1 = 0, current_btn2 = 0, current_wheel_mode = 0x11;
    static uint8_t previous_wheel_mode = 0x11;  // *** HINZUGEFÜGT: Für Mode-Change-Detection ***
    static uint32_t last_button_scan = 0, last_rotary_scan = 0;
    static uint8_t state = 0;

    uint32_t current_time = HAL_GetTick();

    switch (state) {
        case 0: {  // ENCODER (jeder Durchlauf)
            int16_t detents = encoder_read_1ms();
            accumulator += detents;
            state = 1;
            break;
        }

        case 1: {  // BUTTONS (mit Timer)
            if (current_time - last_button_scan >= 25) {
                uint8_t new_btn1, new_btn2;
                button_matrix_scan(&new_btn1, &new_btn2);

                if (new_btn1 != current_btn1 || new_btn2 != current_btn2) {
                    current_btn1 = new_btn1;
                    current_btn2 = new_btn2;
                }
                last_button_scan = current_time;
            }
            state = 2;
            break;
        }

        case 2: { // ROTARY (mit Timer)
            if (current_time - last_rotary_scan >= 50) {
                uint8_t new_wheel_mode = rotary_switch_read();

                if (new_wheel_mode != current_wheel_mode) {
                    // Accumulator zurücksetzen
                    if (accumulator != 0) {
                        accumulator = 0;
                    }

                    // *** NEU: Encoder-Buffer über Funktion zurücksetzen ***
                    encoder_reset_buffers();

                    previous_wheel_mode = current_wheel_mode;
                    current_wheel_mode = new_wheel_mode;
                }
                last_rotary_scan = current_time;
            }
            state = 3;
            break;
        }

        case 3: {  // USB SEND (mit Timer)
            if (current_time - last_send >= 50) {
                int8_t wheel_value = 0;

                if (accumulator != 0) {
                    int16_t abs_acc = (accumulator < 0) ? -accumulator : accumulator;
                    int16_t speed = (abs_acc >= 8) ? 10 : (abs_acc >= 5) ? 6 :
                                   (abs_acc >= 3) ? 3 : (abs_acc >= 2) ? 2 : 1;
                    wheel_value = (accumulator < 0) ? -speed : speed;
                    accumulator = 0;
                }

                in_report.btn_1 = current_btn1;
                in_report.btn_2 = current_btn2;
                in_report.wheel_mode = current_wheel_mode;
                in_report.wheel = wheel_value;
                in_report.xor_day = xhc_get_day() ^ current_btn1;

                USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&in_report, sizeof(in_report));
                last_send = current_time;
            }
            state = 0;
            break;
        }
    }
}

/**
 * @brief Komplett minimale Version - nur Encoder
 * Für Debugging: Testet ob das Problem am Encoder liegt
 */
void xhc_main_loop_encoder_only(void)
{
    static uint8_t btn1 = 0, btn2 = 0, wheel_mode = 0x11;
    static int32_t accumulator = 0;
    static uint32_t last_send = 0;

    uint32_t current_time = HAL_GetTick();

    // Kontinuierlich akkumulieren
    int16_t detents = encoder_read_1ms();
    accumulator += detents;

    // Alle 50ms senden
    if (current_time - last_send >= 50) {
        if (accumulator != 0) {
            // Speed basierend auf Akkumulator
            int16_t abs_acc = (accumulator < 0) ? -accumulator : accumulator;
            int16_t speed = 1;

            if (abs_acc >= 8) speed = 10;
            else if (abs_acc >= 5) speed = 6;
            else if (abs_acc >= 3) speed = 3;
            else if (abs_acc >= 2) speed = 2;
            else speed = 1;

            int16_t result = (accumulator < 0) ? -speed : speed;


            int8_t wheel_value = (result > 127) ? 127 :
                                (result < -127) ? -127 :
                                (int8_t)result;

            xhc_send_input_report(btn1, btn2, wheel_mode, wheel_value);
            accumulator = 0;  // Reset nach dem Senden
        }
        last_send = current_time;
    }
}



//************************** TEST FUNKTIONEN DISPLAY *****************************

/**
 * @brief Display Bottleneck Test - Direkt auf Display ausgeben
 *
 * Messung wo die Zeit verloren geht, Ergebnis auf LCD anzeigen
 */

void measure_display_bottlenecks_on_screen(void)
{
    char test_text[] = "  123.4567";
    char result_line[30];
    uint32_t t1, t2, t3, t4, t5;

    // Display löschen
    fillScreen(WHITE);

    // Titel anzeigen
    ST7735_WriteString(10, 10, "BOTTLENECK TEST", Font_7x10, BLACK, WHITE);

    t1 = HAL_GetTick();

    // Test 1: Nur format_coordinate
    char formatted[20];
    format_coordinate(formatted, 123, 4567, 0);

    t2 = HAL_GetTick();

    // Test 2: FillRectangle (Cache clear)
    ST7735_FillRectangle(65, 100, 95, 15, BLUE);  // Sichtbarer Test-Bereich

    t3 = HAL_GetTick();

    // Test 3: Normale Font
    ST7735_WriteString(65, 100, formatted, Font_7x10, WHITE, BLUE);

    t4 = HAL_GetTick();

    // Test 4: GFX Font (an anderer Stelle)
    ST7735_WriteString_GFX(10, 120, formatted, &dosis_bold8pt7b, BLACK, WHITE);

    t5 = HAL_GetTick();

    // Ergebnisse auf Display ausgeben
    uint32_t format_time = t2 - t1;
    uint32_t fillrect_time = t3 - t2;
    uint32_t normal_font_time = t4 - t3;
    uint32_t gfx_font_time = t5 - t4;
    uint32_t total_time = t5 - t1;

    // Zeile für Zeile anzeigen
    sprintf(result_line, "Format: %lums", format_time);
    ST7735_WriteString(10, 30, result_line, Font_7x10, BLACK, WHITE);

    sprintf(result_line, "FillRect: %lums", fillrect_time);
    ST7735_WriteString(10, 45, result_line, Font_7x10, BLACK, WHITE);

    sprintf(result_line, "Normal Font: %lums", normal_font_time);
    ST7735_WriteString(10, 60, result_line, Font_7x10, BLACK, WHITE);

    sprintf(result_line, "GFX Font: %lums", gfx_font_time);
    ST7735_WriteString(10, 75, result_line, Font_7x10, BLACK, WHITE);

    sprintf(result_line, "TOTAL: %lums", total_time);
    ST7735_WriteString(10, 90, result_line, Font_7x10, RED, WHITE);

    // Fazit anzeigen
    if (gfx_font_time > normal_font_time * 3) {
        ST7735_WriteString(10, 140, "GFX FONT IS SLOW!", Font_7x10, RED, WHITE);
    }
    if (fillrect_time > 10) {
        ST7735_WriteString(10, 155, "FILLRECT IS SLOW!", Font_7x10, RED, WHITE);
    }
}

// Vergleichstest: Verschiedene Font-Methoden
void compare_font_methods(void)
{
    char test_text[] = "X: 123.4567";
    char result[30];
    uint32_t start, end;

    fillScreen(WHITE);
    ST7735_WriteString(10, 5, "FONT SPEED TEST", Font_7x10, BLACK, WHITE);

    // Test 1: Normale Font (7x10)
    start = HAL_GetTick();
    for (int i = 0; i < 10; i++) {  // 10 mal für bessere Messung
        ST7735_WriteString(10, 20, test_text, Font_7x10, BLACK, WHITE);
    }
    end = HAL_GetTick();
    sprintf(result, "Font_7x10: %lums", end - start);
    ST7735_WriteString(10, 40, result, Font_7x10, BLACK, WHITE);

    HAL_Delay(1000);  // 1 Sekunde warten

    // Test 2: GFX Font (dosis_bold8pt7b)
    start = HAL_GetTick();
    for (int i = 0; i < 10; i++) {  // 10 mal für bessere Messung
        ST7735_WriteString_GFX(10, 60, test_text, &dosis_bold8pt7b, BLACK, WHITE);
    }
    end = HAL_GetTick();
    sprintf(result, "GFX Font: %lums", end - start);
    ST7735_WriteString(10, 80, result, Font_7x10, BLACK, WHITE);

    HAL_Delay(1000);  // 1 Sekunde warten

    // Test 3: Nur FillRectangle
    start = HAL_GetTick();
    for (int i = 0; i < 10; i++) {
        ST7735_FillRectangle(10, 100, 100, 15, BLUE);
    }
    end = HAL_GetTick();
    sprintf(result, "10x FillRect: %lums", end - start);
    ST7735_WriteString(10, 120, result, Font_7x10, BLACK, WHITE);

    // Erwartung anzeigen
    ST7735_WriteString(10, 140, "Expected: GFX >> Normal", Font_7x10, RED, WHITE);
}

// Koordinaten-Update Speed-Test
void test_coordinate_update_speed(void)
{
    uint32_t start, end;
    char result[30];

    fillScreen(WHITE);
    ST7735_WriteString(10, 5, "COORD UPDATE TEST", Font_7x10, BLACK, WHITE);

    // Simuliere Koordinaten-Daten
    output_report.pos[0].p_int = 123;
    output_report.pos[0].p_frac = 0x1234;  // Ohne Vorzeichen-Bit
    output_report.pos[1].p_int = 456;
    output_report.pos[1].p_frac = 0x5678;
    output_report.pos[2].p_int = 789;
    output_report.pos[2].p_frac = 0x9ABC;

    // Test: Komplettes Koordinaten-Update
    start = HAL_GetTick();
    xhc_ui_update_coordinates();
    end = HAL_GetTick();

    sprintf(result, "Full Update: %lums", end - start);
    ST7735_WriteString(10, 25, result, Font_7x10, BLACK, WHITE);

    HAL_Delay(2000);

    // Test: Nur eine Koordinate mit normaler Font
    start = HAL_GetTick();
    char text[20];
    format_coordinate(text, 123, 4567, 0);
    ST7735_WriteString(65, 50, text, Font_7x10, BLACK, WHITE);
    end = HAL_GetTick();

    sprintf(result, "1 Coord Normal: %lums", end - start);
    ST7735_WriteString(10, 45, result, Font_7x10, BLACK, WHITE);

    HAL_Delay(1000);

    // Test: Eine Koordinate mit GFX Font
    start = HAL_GetTick();
    ST7735_WriteString_GFX(65, 70, text, &dosis_bold8pt7b, BLACK, WHITE);
    end = HAL_GetTick();

    sprintf(result, "1 Coord GFX: %lums", end - start);
    ST7735_WriteString(10, 65, result, Font_7x10, BLACK, WHITE);

    // Fazit
    ST7735_WriteString(10, 90, "GFX Font = Problem?", Font_7x10, RED, WHITE);
}

// Haupt-Test-Funktion für main.c
void run_display_performance_tests(void)
{
    // Display und Hardware initialisieren
    ST7735_Init(3);
    xhc_ui_init();

    HAL_Delay(2000);  // 2 Sekunden warten

    // Test 1: Bottleneck-Analyse
    measure_display_bottlenecks_on_screen();
    HAL_Delay(5000);  // 5 Sekunden anzeigen

    // Test 2: Font-Vergleich
    compare_font_methods();
    HAL_Delay(5000);  // 5 Sekunden anzeigen

    // Test 3: Koordinaten-Update-Speed
    test_coordinate_update_speed();
    HAL_Delay(5000);  // 5 Sekunden anzeigen

    // Ende
    fillScreen(GREEN);
    ST7735_WriteString(10, 50, "TESTS COMPLETE", Font_7x10, BLACK, GREEN);
    ST7735_WriteString(10, 70, "Check results above", Font_7x10, BLACK, GREEN);
}
