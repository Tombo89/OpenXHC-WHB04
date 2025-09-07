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
                    printf("Buttons: %02X %02X\r\n", current_btn1, current_btn2);
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
                    printf("Rotary change: %02X -> %02X\r\n", current_wheel_mode, new_wheel_mode);

                    // Accumulator zurücksetzen
                    if (accumulator != 0) {
                        printf("Reset accumulator: %ld (mode change)\r\n", accumulator);
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

                    printf("Mode %02X: acc=%ld, speed=%d\r\n", current_wheel_mode, accumulator, wheel_value);
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

            printf("Accumulation: acc=%ld, speed=%d\r\n", accumulator, result);

            int8_t wheel_value = (result > 127) ? 127 :
                                (result < -127) ? -127 :
                                (int8_t)result;

            xhc_send_input_report(btn1, btn2, wheel_mode, wheel_value);
            accumulator = 0;  // Reset nach dem Senden
        }
        last_send = current_time;
    }
}

/**
 * @brief Debug-Funktion: Zeigt aktuellen Zustand
 */
void xhc_debug_current_state(void)
{
    static uint32_t last_debug = 0;
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_debug >= 1000) {  // Alle 1 Sekunde
        printf("XHC State: BTN1=0x%02X BTN2=0x%02X MODE=0x%02X\r\n",
               current_btn1, current_btn2, current_wheel_mode);
        last_debug = current_time;
    }
}


/**
 * @brief Debug-Callback für empfangene Daten
 */
void xhc_debug_callback(void)
{
    /* Beispiel: LED toggeln wenn Daten empfangen */
    #ifdef DEBUG_LED_GPIO_Port
    HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin);
    #endif

    /* Debug-Ausgabe (falls verfügbar) */
    #ifdef XHC_DEBUG_UART
    // printf implementierung falls UART verfügbar
    #endif
}
