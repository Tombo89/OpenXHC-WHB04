/*
 * Encoder Implementation für STM32CubeIDE
 * Portiert vom Original Keil-Code
 */

#include "main.h"
#include "xhc_main.h"
#include "ST7735.h"

/* Encoder Hardware-Konfiguration */
#define ENCODER_TIMER           TIM2
#define ENCODER_TIMER_CLK_EN()  __HAL_RCC_TIM2_CLK_ENABLE()

/* Globale Variablen für Encoder */
static TIM_HandleTypeDef htim_encoder;
static uint16_t enc_prev_cnt = 0;
static int32_t enc_rem = 0; /* Rest-Impulse (0..3) */

/**
 * @brief Encoder Hardware initialisieren
 *
 * WICHTIG: Pins müssen in CubeMX konfiguriert werden:
 * - PA15: TIM2_CH1 (Encoder Input)
 * - PB3:  TIM2_CH2 (Encoder Input)
 * - JTAG deaktivieren um PA15/PB3 freizugeben
 */
void encoder_init(void)
{
    TIM_Encoder_InitTypeDef encoder_config = {0};
    TIM_MasterConfigTypeDef master_config = {0};

    /* Timer Clock aktivieren */
    ENCODER_TIMER_CLK_EN();

    /* Encoder Timer Handle konfigurieren */
    htim_encoder.Instance = ENCODER_TIMER;
    htim_encoder.Init.Prescaler = 0;
    htim_encoder.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim_encoder.Init.Period = 0xFFFF; /* 16-bit Counter */
    htim_encoder.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim_encoder.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    /* Encoder Konfiguration */
    encoder_config.EncoderMode = TIM_ENCODERMODE_TI12; /* Beide Kanäle, beide Flanken */
    encoder_config.IC1Polarity = TIM_ICPOLARITY_RISING;
    encoder_config.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    encoder_config.IC1Prescaler = TIM_ICPSC_DIV1;
    encoder_config.IC1Filter = 0x04; /* Entprellung */
    encoder_config.IC2Polarity = TIM_ICPOLARITY_RISING;
    encoder_config.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    encoder_config.IC2Prescaler = TIM_ICPSC_DIV1;
    encoder_config.IC2Filter = 0x04; /* Entprellung */

    /* Timer als Encoder initialisieren */
    if (HAL_TIM_Encoder_Init(&htim_encoder, &encoder_config) != HAL_OK) {
        Error_Handler();
    }

    /* Master Config (optional) */
    master_config.MasterOutputTrigger = TIM_TRGO_RESET;
    master_config.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim_encoder, &master_config) != HAL_OK) {
        Error_Handler();
    }

    /* Timer starten */
    if (HAL_TIM_Encoder_Start(&htim_encoder, TIM_CHANNEL_ALL) != HAL_OK) {
        Error_Handler();
    }

    /* Startwert merken */
    enc_prev_cnt = (uint16_t)__HAL_TIM_GET_COUNTER(&htim_encoder);
    enc_rem = 0;
}

/**
 * @brief Encoder-Wert lesen (Rastungen)
 * @return Anzahl Rastungen seit letztem Aufruf (-127 bis +127)
 */
int16_t encoder_read(void)
{
    uint16_t cur = (uint16_t)__HAL_TIM_GET_COUNTER(&htim_encoder);

    /* Differenz als signed behandelt - funktioniert bei Wrap-around */
    int16_t diff = (int16_t)(cur - enc_prev_cnt);
    enc_prev_cnt = cur;

    /* Impulse akkumulieren */
    enc_rem += diff;

    /* 4 Impulse = 1 Rastung */
    int16_t detents = (int16_t)(enc_rem / 4);
    enc_rem -= (int32_t)detents * 4; /* Rest für nächste Abfrage */

    return detents;
}

/**
 * @brief Encoder zurücksetzen
 */
void encoder_reset(void)
{
    __HAL_TIM_SET_COUNTER(&htim_encoder, 0);
    enc_prev_cnt = 0;
    enc_rem = 0;
}

/**
 * @brief Test-Funktion: Encoder-Werte auf Display anzeigen
 */
void encoder_display_test(void)
{
    static int32_t total_detents = 0;
    int16_t new_detents = encoder_read();

    if (new_detents != 0) {
        total_detents += new_detents;
    }

    char text[32];
    sprintf(text, "Enc: %d (%d)", (int)new_detents, (int)total_detents);
    ST7735_WriteString(0, 130, text, Font_7x10, RED, BLACK);

    /* Rohdaten anzeigen */
    uint16_t raw_count = (uint16_t)__HAL_TIM_GET_COUNTER(&htim_encoder);
    sprintf(text, "Raw: %d Rem: %d", raw_count, (int)enc_rem);
    ST7735_WriteString(0, 140, text, Font_7x10, RED, BLACK);
}

/*
 * INTEGRATION IN CUBEMX:
 *
 * 1. In CubeMX:
 *    - System Core -> SYS -> Debug: Serial Wire (um JTAG zu deaktivieren)
 *    - Timers -> TIM2 -> Combined Channels -> Encoder Mode
 *    - Pinout: PA15 = TIM2_CH1, PB3 = TIM2_CH2
 *    - Parameter Settings:
 *      * Encoder Mode: TI1 and TI2
 *      * Counter Period: 65535
 *      * Input Filter CH1/CH2: 4
 *
 * 2. Code generieren
 *
 * 3. In main.c nach MX_TIM2_Init():
 *    encoder_init();
 *
 * 4. In main.c while-Schleife:
 *    encoder_display_test();
 *
 * 5. In xhc_main_loop() echte Encoder-Werte verwenden:
 *    int8_t wheel_value = (int8_t)encoder_read(); // Statt 0
 */
