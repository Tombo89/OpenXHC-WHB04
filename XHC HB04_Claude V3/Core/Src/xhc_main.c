/*
 * XHC HB04 Main Implementation für STM32CubeIDE
 */

#include "xhc_main.h"
#include "main.h"
#include <string.h>

/* Externe Variablen */
extern USBD_HandleTypeDef hUsbDeviceFS;

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
    /* Fülle die Input Report Struktur */
    in_report.btn_1 = btn1;
    in_report.btn_2 = btn2;
    in_report.wheel_mode = wheel_mode;
    in_report.wheel = wheel_value;
    in_report.xor_day = xhc_get_day() ^ btn1;  // XOR-Verschlüsselung

    /* Sende Report über Custom HID */
    if (USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&in_report, sizeof(in_report)) == USBD_OK)
    {
        return USBD_OK;
    }

    return USBD_FAIL;
}

/**
 * @brief XHC Hauptschleife für die Anwendung
 */
void xhc_main_loop(void)
{
    static uint32_t last_send_time = 0;
    uint32_t current_time = HAL_GetTick();

    /* Sende Input Report alle 10ms */
    if (current_time - last_send_time >= 10)
    {
        /* TODO: Hier später echte Input-Daten lesen:
         * - Tasteneingaben vom Keyboard-Matrix
         * - Encoder-Werte
         * - Rotary-Switch Position
         */

        uint8_t btn1 = 0;           // Keine Taste gedrückt
        uint8_t btn2 = 0;           // Keine Taste gedrückt
        uint8_t wheel_mode = 0x11;  // X-Achse
        int8_t wheel_value = 0;     // Kein Encoder-Movement

        /* Sende Input Report */
        xhc_send_input_report(btn1, btn2, wheel_mode, wheel_value);

        last_send_time = current_time;
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
