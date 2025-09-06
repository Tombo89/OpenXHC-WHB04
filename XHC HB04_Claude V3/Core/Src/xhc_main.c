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
 * @brief XHC Hauptschleife für die Anwendung - mit 1ms Encoder-Abtastung
 */
void xhc_main_loop(void)
{
    static uint8_t last_btn1 = 0;
    static uint8_t last_btn2 = 0;
    static uint8_t last_wheel_mode = 0x11;

    uint8_t state_changed = 0;  // Flag für Änderungen

    /* Aktuelle Input-Daten lesen */
    uint8_t btn1, btn2;
    button_matrix_scan(&btn1, &btn2);  // Echte Button-Matrix

    uint8_t wheel_mode = rotary_switch_read();  // Echter Rotary Switch

    /* 1ms Encoder-Wert lesen */
    int16_t encoder_detents = encoder_read_1ms();  // Neue 1ms Funktion
    int8_t wheel_value = 0;

    /* Encoder-Wert begrenzen */
    if (encoder_detents > 127) {
        wheel_value = 127;
    } else if (encoder_detents < -127) {
        wheel_value = -127;
    } else {
        wheel_value = (int8_t)encoder_detents;
    }


    /* Prüfe auf Änderungen */

    // Encoder-Änderung? (Ändere die Bedingung)
    if (encoder_1ms_has_data() || wheel_value != 0) {  // Beide Bedingungen prüfen
        state_changed = 1;
    }

    // Tasten-Änderung?
    if (btn1 != last_btn1 || btn2 != last_btn2) {
        state_changed = 1;
        last_btn1 = btn1;
        last_btn2 = btn2;
    }

    // Wheel-Mode-Änderung?
    if (wheel_mode != last_wheel_mode) {
        state_changed = 1;
        last_wheel_mode = wheel_mode;
    }


    /* Sende NUR bei echten Änderungen */
    if (state_changed) {
        xhc_send_input_report(btn1, btn2, wheel_mode, wheel_value);


        /* Zähler für gesendete Pakete */
        static uint32_t tx_counter = 0;
        tx_counter++;
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
