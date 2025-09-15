/*
 * XHC HB04 Data Structures for STM32CubeIDE
 * Portiert vom Keil uVision Projekt
 */

#ifndef XHC_DATASTRUCTURES_H
#define XHC_DATASTRUCTURES_H

#include <stdint.h>

#define WHBxx_MAGIC 0xFDFE
#define DEV_UNKNOW 0
#define DEV_WHB04  sizeof(struct whb04_out_data)

#define WHBxx_VID 0x10CE
#define WHB04_PID 0xEB70

#pragma pack(push, 1)

/* Host→Device Datenstrukturen */

struct whb04_out_data
{
    /* Header des Pakets */
    uint16_t    magic;      // 0xFDFE
    uint8_t     day;        // Tag des Monats (XOR-Schlüssel)

    /* Positionsdaten - 6 Achsen */
    struct {
        uint16_t    p_int;   // Integer-Teil der Position
        uint16_t    p_frac;  // Fraktionaler Teil (16-bit)
    } pos[6];

    /* Geschwindigkeiten */
    uint16_t    feedrate_ovr;    // Feedrate Override
    uint16_t    sspeed_ovr;      // Spindle Speed Override
    uint16_t    feedrate;        // Aktuelle Feedrate
    uint16_t    sspeed;          // Aktuelle Spindle Speed

    uint8_t     step_mul;        // Schrittmultiplikator
    uint8_t     state;          // Maschinenstatus
};

/* Device→Host Datenstruktur */
struct whb0x_in_data
{
    uint8_t     id;         // Report ID (0x04)
    uint8_t     btn_1;      // Taste 1
    uint8_t     btn_2;      // Taste 2
    uint8_t     wheel_mode; // Rad-Modus (Achsenauswahl)
    int8_t      wheel;      // Rad-Geschwindigkeit und -Richtung
    uint8_t     xor_day;    // Tag XOR Taste 1 (Verschlüsselung)
};

#pragma pack(pop)

/* Globale Variablen */
extern struct whb04_out_data output_report;
extern struct whb0x_in_data in_report;
extern uint8_t day;  // XOR-Schlüssel

/* Funktionsprototypen */
void xhc_recv(uint8_t *data);
void xhc_process_received_data(void);

#endif /* XHC_DATASTRUCTURES_H */
