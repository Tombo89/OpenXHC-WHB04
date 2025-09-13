/*
 * XHC HB04 Datenempfang für STM32CubeIDE
 * Portiert vom Keil uVision Projekt
 */

#include "XHC_DataStructures.h"
#include <string.h>
#include "ST7735.h"
#include <stdio.h>
#include "xhc_display_ui.h"
#include "rotary_switch.h"

/* Konstanten für den Empfang */
#define TMP_BUFF_SIZE   42
#define CHUNK_SIZE      7
#define HW_TYPE         DEV_WHB04  // Nur WHB04 (37 Bytes)

/* Globale Variablen */
struct whb04_out_data output_report = { 0 };
struct whb0x_in_data in_report = { .id = 0x04 };
uint8_t day = 0;  // XOR-Schlüssel

/* Statische Variablen für State Machine */
static int offset = 0;
static uint8_t magic_found = 0;
static uint8_t tmp_buff[TMP_BUFF_SIZE];

/**
 * @brief Empfängt Daten vom Host über HID SET_REPORT
 * @param data Zeiger auf die empfangenen 7-Byte-Chunks
 *
 * Diese Funktion implementiert eine State Machine um die 37-Byte-Payload
 * aus mehreren 7-Byte-Chunks zu rekonstruieren.
 */
void xhc_recv(uint8_t *data)
{
    /* Prüfe auf Magic-Wert am Anfang eines neuen Pakets */
    if (*(uint16_t*)data == WHBxx_MAGIC)
    {
        offset = 0;
        magic_found = 1;
    }

    /* Wenn kein Magic gefunden wurde, verwerfe die Daten */
    if (!magic_found)
        return;

    /* Pufferüberlauf vermeiden */
    if ((offset + CHUNK_SIZE) > TMP_BUFF_SIZE)
        return;

    /* Kopiere den 7-Byte-Chunk in den temporären Puffer */
    memcpy(&tmp_buff[offset], data, CHUNK_SIZE);
    offset += CHUNK_SIZE;

    /* Prüfe ob wir alle Daten empfangen haben (37 Bytes für WHB04) */
    if (offset < HW_TYPE)
        return;

    /* Alle Daten empfangen - verarbeite das Paket */
    magic_found = 0;

    /* Kopiere die empfangenen Daten in die output_report Struktur */
    output_report = *((struct whb04_out_data*)tmp_buff);

    /* Aktualisiere den XOR-Schlüssel */
    day = output_report.day;

    /* Signalisiere dass neue Daten verfügbar sind */
    xhc_process_received_data();
}

/**
 * @brief Verarbeitet die empfangenen Daten
 *
 * Diese Funktion wird aufgerufen wenn ein vollständiges Datenpaket
 * empfangen wurde. Hier können die Daten weiterverarbeitet werden
 * (z.B. LCD-Update, LED-Steuerung, etc.)
 */
void xhc_process_received_data(void)
{
    static uint32_t last_position_values[6] = {0};
    static uint32_t last_display_update = 0;
    static uint32_t update_count = 0;
    static uint8_t settling_mode = 1;  // Anfangs mehr Updates

    uint32_t current_time = HAL_GetTick();

    uint8_t need_update = 0;
    uint8_t any_change = 0;

    // Prüfe jede Achse einzeln
    for (int i = 0; i < 6; i++) {
        uint32_t current_value = (output_report.pos[i].p_int << 16) |
                                (output_report.pos[i].p_frac & 0x7FFF);

        uint32_t diff = (current_value > last_position_values[i]) ?
                       (current_value - last_position_values[i]) :
                       (last_position_values[i] - current_value);

        // *** ADAPTIVE SCHWELLE ***
        uint32_t threshold = settling_mode ? 1 : 10;  // Anfangs sehr empfindlich

        if (diff >= threshold) {
            any_change = 1;
            last_position_values[i] = current_value;
        }
    }

    // Update-Entscheidung
    if (any_change) {
        need_update = 1;
        update_count++;

        // Nach 20 Updates weniger empfindlich werden
        if (update_count >= 20) {
            settling_mode = 0;

        }
    }
    // Fallback: Alle 100ms
    else if ((current_time - last_display_update) >= 100) {
        need_update = 1;
    }

    if (need_update) {
        xhc_ui_update_coordinates();
        last_display_update = current_time;
    }

    if ((current_time - last_display_update) >= 500) {
            xhc_ui_update_status_bar(rotary_switch_read(), output_report.step_mul);
        }
        last_display_update = current_time;
    }


/**
 * @brief Hilfsfunktion um die empfangenen Positionsdaten zu extrahieren
 * @param axis Achse (0=X, 1=Y, 2=Z, 3=A, 4=B, 5=C)
 * @param is_machine 0=Workspace-Koordinaten, 1=Maschinen-Koordinaten
 * @return Positionswert als Float
 */
float xhc_get_position(uint8_t axis, uint8_t is_machine)
{
    if (axis >= 6) return 0.0f;

    /* Für WHB04: 6 Achsen, erst WC dann MC */
    uint8_t pos_index = axis + (is_machine ? 3 : 0);
    if (pos_index >= 6) return 0.0f;

    float result = (float)output_report.pos[pos_index].p_int;

    /* Fraktionaler Teil für WHB04 (16-bit) */
    uint16_t frac = output_report.pos[pos_index].p_frac;
    uint8_t sign = (frac & 0x8000) ? 1 : 0;  // MSB = Vorzeichen
    frac &= 0x7FFF;  // Vorzeichen entfernen

    result += (float)frac / 10000.0f;

    if (sign)
        result = -result;

    return result;
}

/**
 * @brief Gibt den aktuellen Maschinenstatus zurück
 * @return Status-Byte
 */
uint8_t xhc_get_machine_state(void)
{
    return output_report.state;
}

/**
 * @brief Gibt den aktuellen Schrittmultiplikator zurück
 * @return Step-Multiplikator
 */
uint8_t xhc_get_step_multiplier(void)
{
    return output_report.step_mul;
}
