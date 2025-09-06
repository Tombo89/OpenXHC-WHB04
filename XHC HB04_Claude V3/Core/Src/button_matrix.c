/*
 * Button Matrix Implementation für STM32CubeIDE
 * Portiert vom Original kbd_driver.c
 */

#include "main.h"
#include "button_matrix.h"
#include "ST7735.h"
#include "stdio.h"

/* Button-Matrix Hardware-Konfiguration */
/* COLs (Spalten) - Input Pull-Up */
#define COL1_GPIO_Port  GPIOB
#define COL1_Pin        GPIO_PIN_5
#define COL2_GPIO_Port  GPIOB
#define COL2_Pin        GPIO_PIN_6
#define COL3_GPIO_Port  GPIOB
#define COL3_Pin        GPIO_PIN_7
#define COL4_GPIO_Port  GPIOB
#define COL4_Pin        GPIO_PIN_8

/* ROWs (Zeilen) - Output Open-Drain */
#define ROW1_GPIO_Port  GPIOB
#define ROW1_Pin        GPIO_PIN_12
#define ROW2_GPIO_Port  GPIOB
#define ROW2_Pin        GPIO_PIN_13
#define ROW3_GPIO_Port  GPIOB
#define ROW3_Pin        GPIO_PIN_14
#define ROW4_GPIO_Port  GPIOB
#define ROW4_Pin        GPIO_PIN_15

/* XHC Button Codes aus der PDF und dem Original-Code */
#define BTN_Reset       0x17
#define BTN_Stop        0x16
#define BTN_Goto0       0x01
#define BTN_SS          0x02    // Start/Stop
#define BTN_Rewind      0x03
#define BTN_ProbeZ      0x04
#define BTN_Spindle     0x0C
#define BTN_Half        0x06    // 1/2
#define BTN_Zero        0x07    // 0
#define BTN_SafeZ       0x08
#define BTN_GotoHome    0x09
#define BTN_Macro1      0x0A
#define BTN_Macro2      0x0B
#define BTN_Macro3      0x05
#define BTN_Macro6      0x0F
#define BTN_Macro7      0x10
#define BTN_Step        0x0D    // Step+
#define BTN_MPG         0x0E

/* 4x4 Key-Matrix Layout (wie im Original - vertikal gespiegelt wegen PCB) */
static const uint8_t kbd_key_codes[] =
{
    /* Zeile 0: reset, stop, m1, m2 */
    BTN_SS,      BTN_Zero,    BTN_Macro1,  BTN_Stop,
    /* Zeile 1: GoZero, s/p, rewind, Probe-Z */
    BTN_Spindle, BTN_SafeZ,   BTN_ProbeZ,  BTN_Goto0,
    /* Zeile 2: Spind, =1/2, =0, Safe-Z */
    BTN_Macro7,  BTN_Macro6,  BTN_Macro3,  BTN_GotoHome,
    /* Zeile 3: Home, M6, Step+, M3 */
    BTN_Macro2,  BTN_Rewind,  BTN_Half,    BTN_Step,
};

/* Globale Variablen für Entprellung */
static uint8_t last_btn1 = 0;
static uint8_t last_btn2 = 0;
static uint32_t last_scan_time = 0;

/**
 * @brief Button-Matrix Hardware initialisieren
 *
 * WICHTIG: Pins müssen in CubeMX konfiguriert werden:
 * - B5,B6,B7,B8: GPIO_Input mit Pull-Up
 * - B12,B13,B14,B15: GPIO_Output_OD (Open Drain)
 */
void button_matrix_init(void)
{
    /* GPIO Clock ist bereits von CubeMX aktiviert */

    /* Alle Zeilen auf High-Z setzen (Open-Drain High) */
    HAL_GPIO_WritePin(ROW1_GPIO_Port, ROW1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ROW2_GPIO_Port, ROW2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ROW3_GPIO_Port, ROW3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ROW4_GPIO_Port, ROW4_Pin, GPIO_PIN_SET);
}

/**
 * @brief Liest den Status einer Spalte
 * @param col Spaltennummer (0-3)
 * @return 1 wenn Taste nicht gedrückt, 0 wenn gedrückt
 */
static uint8_t read_column(uint8_t col)
{
    switch (col) {
        case 0: return HAL_GPIO_ReadPin(COL1_GPIO_Port, COL1_Pin);
        case 1: return HAL_GPIO_ReadPin(COL2_GPIO_Port, COL2_Pin);
        case 2: return HAL_GPIO_ReadPin(COL3_GPIO_Port, COL3_Pin);
        case 3: return HAL_GPIO_ReadPin(COL4_GPIO_Port, COL4_Pin);
        default: return 1;
    }
}

/**
 * @brief Setzt eine Zeile auf Low (aktiviert)
 * @param row Zeilennummer (0-3)
 */
static void set_row_low(uint8_t row)
{
    switch (row) {
        case 0: HAL_GPIO_WritePin(ROW1_GPIO_Port, ROW1_Pin, GPIO_PIN_RESET); break;
        case 1: HAL_GPIO_WritePin(ROW2_GPIO_Port, ROW2_Pin, GPIO_PIN_RESET); break;
        case 2: HAL_GPIO_WritePin(ROW3_GPIO_Port, ROW3_Pin, GPIO_PIN_RESET); break;
        case 3: HAL_GPIO_WritePin(ROW4_GPIO_Port, ROW4_Pin, GPIO_PIN_RESET); break;
    }
}

/**
 * @brief Setzt eine Zeile auf High-Z (deaktiviert)
 * @param row Zeilennummer (0-3)
 */
static void set_row_high(uint8_t row)
{
    switch (row) {
        case 0: HAL_GPIO_WritePin(ROW1_GPIO_Port, ROW1_Pin, GPIO_PIN_SET); break;
        case 1: HAL_GPIO_WritePin(ROW2_GPIO_Port, ROW2_Pin, GPIO_PIN_SET); break;
        case 2: HAL_GPIO_WritePin(ROW3_GPIO_Port, ROW3_Pin, GPIO_PIN_SET); break;
        case 3: HAL_GPIO_WritePin(ROW4_GPIO_Port, ROW4_Pin, GPIO_PIN_SET); break;
    }
}

/**
 * @brief Scannt die Button-Matrix und gibt gedrückte Tasten zurück
 * @param btn1 Zeiger für erste gedrückte Taste (Output)
 * @param btn2 Zeiger für zweite gedrückte Taste (Output, kann NULL sein)
 * @return Anzahl gedrückter Tasten (0, 1 oder 2)
 */
uint8_t button_matrix_scan(uint8_t *btn1, uint8_t *btn2)
{
    uint8_t pressed_count = 0;

    /* Entprellung: Mindestens 20ms zwischen Scans */
    uint32_t current_time = HAL_GetTick();
    if (current_time - last_scan_time < 20) {
        *btn1 = last_btn1;
        if (btn2) *btn2 = last_btn2;
        return (last_btn1 != 0) ? ((last_btn2 != 0) ? 2 : 1) : 0;
    }
    last_scan_time = current_time;

    /* Initialisiere Ausgabewerte */
    *btn1 = 0;
    if (btn2) *btn2 = 0;

    /* Scanne jede Zeile */
    for (uint8_t row = 0; row < 4; row++) {
        /* Aktiviere aktuelle Zeile (Low) */
        set_row_low(row);

        /* Kurze Verzögerung für Signal-Stabilisierung */
        for (volatile int i = 0; i < 50; i++) __NOP();

        /* Scanne alle Spalten in dieser Zeile */
        for (uint8_t col = 0; col < 4; col++) {
            if (read_column(col) == GPIO_PIN_RESET) {  /* Taste gedrückt */
                uint8_t key_code = kbd_key_codes[(row * 4) + col];

                if (pressed_count == 0) {
                    *btn1 = key_code;
                    pressed_count = 1;
                } else if (btn2 && pressed_count == 1) {
                    *btn2 = key_code;
                    pressed_count = 2;
                    /* Deaktiviere Zeile und return bei 2 Tasten */
                    set_row_high(row);
                    goto scan_complete;
                }
            }
        }

        /* Deaktiviere aktuelle Zeile (High-Z) */
        set_row_high(row);
    }

scan_complete:
    /* Speichere für Entprellung */
    last_btn1 = *btn1;
    last_btn2 = btn2 ? *btn2 : 0;

    return pressed_count;
}

/**
 * @brief Test-Funktion: Zeigt Button-Status auf Display mit mehr Details
 */
void button_matrix_display_test(void)
{
    uint8_t btn1, btn2;
    uint8_t count = button_matrix_scan(&btn1, &btn2);

    char text[32];
    sprintf(text, "Buttons: %d", count);
    ST7735_WriteString(0, 70, text, Font_7x10, CYAN, BLACK);

    if (count > 0) {
        sprintf(text, "Btn1: 0x%02X", btn1);
        ST7735_WriteString(0, 80, text, Font_7x10, CYAN, BLACK);

        if (count > 1) {
            sprintf(text, "Btn2: 0x%02X", btn2);
            ST7735_WriteString(0, 90, text, Font_7x10, CYAN, BLACK);
        } else {
            ST7735_WriteString(0, 90, "Btn2: --", Font_7x10, CYAN, BLACK);
        }
    } else {
        ST7735_WriteString(0, 80, "Btn1: --", Font_7x10, CYAN, BLACK);
        ST7735_WriteString(0, 90, "Btn2: --", Font_7x10, CYAN, BLACK);
    }

    /* Zeige last_btn Werte für Debug */
    extern uint8_t last_btn1, last_btn2;  // Zugriff auf static vars
    sprintf(text, "Last: %02X %02X", last_btn1, last_btn2);
    ST7735_WriteString(0, 100, text, Font_7x10, WHITE, BLACK);
}

/*
 * INTEGRATION IN CUBEMX:
 *
 * 1. CubeMX Konfiguration:
 *    - PB5, PB6, PB7, PB8: GPIO_Input, Pull-up, High speed
 *    - PB12, PB13, PB14, PB15: GPIO_Output, Open Drain, High speed
 *
 * 2. In main.c nach GPIO-Init:
 *    button_matrix_init();
 *
 * 3. In main.c while-Schleife für Test:
 *    button_matrix_display_test();
 *
 * 4. In xhc_main_loop() für echte Integration:
 *    uint8_t count = button_matrix_scan(&btn1, &btn2);
 */
