/**
 * @brief XHC Hauptschleife für die Anwendung
 *
 * Diese Funktion sollte in der main() while-Schleife aufgerufen werden
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
 *
 * Diese Funktion wird von xhc_process_received_data() aufgerufen
 * und kann für Debug-Zwecke verwendet werden.
 */
void xhc_debug_callback(void)
{
    /* Beispiel: LED toggeln wenn Daten empfangen */
    #ifdef DEBUG_LED_GPIO_Port
    HAL_GPIO_TogglePin(DEBUG_LED_GPIO_Port, DEBUG_LED_Pin);
    #endif

    /* Debug-Ausgabe (falls UART aktiviert) */
    #ifdef DEBUG_UART
    printf("XHC: Magic=0x%04X Day=%d X=%d.%d\r\n",
           output_report.magic,
           output_report.day,
           output_report.pos[0].p_int,
           output_report.pos[0].p_frac);
    #endif
}
