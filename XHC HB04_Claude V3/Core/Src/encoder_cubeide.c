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

/* Globale Variablen für 1ms Abtastung */
static volatile int16_t encoder_1ms_buffer = 0;
static volatile uint8_t encoder_data_ready = 0;

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
 * @brief Encoder-Wert lesen (Rastungen) - optimiert für Responsivität
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

    /* Begrenzen auf USB-Übertragungsbereich */
    if (detents > 127) detents = 127;
    if (detents < -127) detents = -127;

    return detents;
}

/**
 * @brief Prüft ob Encoder-Aktivität vorhanden ist (ohne Wert zu konsumieren)
 * @return 1 wenn Encoder bewegt wurde, 0 wenn keine Änderung
 */
uint8_t encoder_has_activity(void)
{
    uint16_t cur = (uint16_t)__HAL_TIM_GET_COUNTER(&htim_encoder);
    return (cur != enc_prev_cnt) ? 1 : 0;
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

/**
 * @brief Zeigt gesendete Encoder-Werte an (für USB-Debug)
 */
void encoder_display_sent_values(void)
{
    static int16_t last_sent_value = 0;
    static uint32_t send_counter = 0;

    int16_t current_detents = encoder_read();

    if (current_detents != 0) {
        last_sent_value = current_detents;
        send_counter++;
    }

    char text[32];
    sprintf(text, "Sent: %d (#%d)", (int)last_sent_value, (int)send_counter);
    ST7735_WriteString(0, 150, text, Font_7x10, GREEN, BLACK);
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


/**
 * @brief 1ms Encoder-Abtastung (wird von SysTick aufgerufen)
 *
 * WICHTIG: Dieser Code läuft im Interrupt-Kontext!
 * Deshalb nur minimale, schnelle Operationen.
 */
void encoder_1ms_poll(void)
{
    static uint16_t last_count = 0;
    static int32_t impulse_buffer = 0;

    /* Schnelle Register-Abfrage */
    uint16_t current_count = TIM2->CNT;
    int16_t diff = (int16_t)(current_count - last_count);
    last_count = current_count;

    if (diff != 0) {
        /* Impulse sammeln */
        impulse_buffer += diff;

        /* 4 Impulse = 1 Rastung */
        int16_t detents = (int16_t)(impulse_buffer / 4);
        if (detents != 0) {
            impulse_buffer -= detents * 4;

            /* Atomic update */
            __disable_irq();
            encoder_1ms_buffer += detents;
            encoder_data_ready = 1;
            __enable_irq();
        }
    }
}

/**
 * @brief Encoder-Werte aus 1ms-Abtastung lesen
 * @return Akkumulierte Rastungen seit letztem Aufruf
 */
int16_t encoder_read_1ms(void)
{
    int16_t result = 0;

    /* Atomic read */
    __disable_irq();
    if (encoder_data_ready) {
        result = encoder_1ms_buffer;
        encoder_1ms_buffer = 0;
        encoder_data_ready = 0;
    }
    __enable_irq();

    /* Begrenzen */
    if (result > 127) result = 127;
    if (result < -127) result = -127;

    return result;
}

/**
 * @brief Prüft ob neue Encoder-Daten verfügbar
 */
uint8_t encoder_1ms_has_data(void)
{
    return encoder_data_ready;
}
