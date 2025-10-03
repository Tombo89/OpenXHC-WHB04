#include "main.h"
#include "rotary_switch.h"
#include "ST7735.h"
#include <stdio.h>

/* Hardware-Definitionen */
#define ROT_POS1_GPIO_Port  GPIOB
#define ROT_POS1_Pin        GPIO_PIN_1   // A
#define ROT_POS2_GPIO_Port  GPIOB
#define ROT_POS2_Pin        GPIO_PIN_11   // F
#define ROT_POS3_GPIO_Port  GPIOB
#define ROT_POS3_Pin        GPIO_PIN_10  // S
#define ROT_POS4_GPIO_Port  GPIOA
#define ROT_POS4_Pin        GPIO_PIN_10  // Z
#define ROT_POS5_GPIO_Port  GPIOA
#define ROT_POS5_Pin        GPIO_PIN_9  // Y
#define ROT_POS6_GPIO_Port  GPIOA
#define ROT_POS6_Pin        GPIO_PIN_8   // X

void rotary_switch_init(void)
{
    /* GPIO Clock bereits von CubeMX aktiviert */
    /* Pins sind bereits als Input Pull-Up konfiguriert */
}

uint8_t rotary_switch_read(void)
{
    static uint32_t off_start_time = 0;
    static uint8_t stable_position = ROTARY_OFF;

    /* Aktuelle Hardware-Position lesen */
    uint8_t current_hw_pos = ROTARY_OFF;

    if (!HAL_GPIO_ReadPin(ROT_POS1_GPIO_Port, ROT_POS1_Pin))
        current_hw_pos = ROTARY_X;
    else if (!HAL_GPIO_ReadPin(ROT_POS2_GPIO_Port, ROT_POS2_Pin))
        current_hw_pos = ROTARY_Y;
    else if (!HAL_GPIO_ReadPin(ROT_POS3_GPIO_Port, ROT_POS3_Pin))
        current_hw_pos = ROTARY_Z;
    else if (!HAL_GPIO_ReadPin(ROT_POS4_GPIO_Port, ROT_POS4_Pin))
        current_hw_pos = ROTARY_SPINDLE;
    else if (!HAL_GPIO_ReadPin(ROT_POS5_GPIO_Port, ROT_POS5_Pin))
        current_hw_pos = ROTARY_FEED;
    else if (!HAL_GPIO_ReadPin(ROT_POS6_GPIO_Port, ROT_POS6_Pin))
        current_hw_pos = ROTARY_A;
    // else bleibt ROTARY_OFF

    /* Entprellung und OFF-Verzögerung */
    if (current_hw_pos != ROTARY_OFF) {
        /* Gültige Position erkannt */
        stable_position = current_hw_pos;
        off_start_time = 0;  // Reset OFF-Timer
    } else {
        /* OFF erkannt - starte Timer wenn noch nicht gestartet */
        if (off_start_time == 0) {
            off_start_time = HAL_GetTick();
        }

        /* Warte 100ms bevor OFF akzeptiert wird */
        if ((HAL_GetTick() - off_start_time) < 100) {
            /* Noch in Verzögerung - gib letzte gültige Position zurück */
            return stable_position;
        }
        /* Nach 100ms OFF akzeptieren */
        stable_position = ROTARY_OFF;
    }

    return stable_position;
}

void rotary_switch_display_test(void)
{
    uint8_t pos = rotary_switch_read();
    char text[32];

    sprintf(text, "Rot: 0x%02X", pos);
    ST7735_WriteString(0, 110, text, Font_7x10, YELLOW, BLACK);

    const char* pos_name;
    switch(pos) {
        case ROTARY_X: pos_name = "X-AXIS"; break;
        case ROTARY_Y: pos_name = "Y-AXIS"; break;
        case ROTARY_Z: pos_name = "Z-AXIS"; break;
        case ROTARY_SPINDLE: pos_name = "SPINDLE"; break;
        case ROTARY_FEED: pos_name = "FEED"; break;
        case ROTARY_A: pos_name = "A-AXIS"; break;
        default: pos_name = "OFF"; break;
    }

    sprintf(text, "Pos: %s", pos_name);
    ST7735_WriteString(0, 120, text, Font_7x10, YELLOW, BLACK);

    /* Debug: Zeige Hardware-Pins direkt */
    uint8_t pin_states = 0;
    if (!HAL_GPIO_ReadPin(ROT_POS1_GPIO_Port, ROT_POS1_Pin)) pin_states |= 0x01;
    if (!HAL_GPIO_ReadPin(ROT_POS2_GPIO_Port, ROT_POS2_Pin)) pin_states |= 0x02;
    if (!HAL_GPIO_ReadPin(ROT_POS3_GPIO_Port, ROT_POS3_Pin)) pin_states |= 0x04;
    if (!HAL_GPIO_ReadPin(ROT_POS4_GPIO_Port, ROT_POS4_Pin)) pin_states |= 0x08;
    if (!HAL_GPIO_ReadPin(ROT_POS5_GPIO_Port, ROT_POS5_Pin)) pin_states |= 0x10;
    if (!HAL_GPIO_ReadPin(ROT_POS6_GPIO_Port, ROT_POS6_Pin)) pin_states |= 0x20;

    sprintf(text, "HW: 0x%02X", pin_states);
    ST7735_WriteString(0, 130, text, Font_7x10, WHITE, BLACK);
}
