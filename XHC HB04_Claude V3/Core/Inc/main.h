/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LCD_RST_Pin GPIO_PIN_2
#define LCD_RST_GPIO_Port GPIOA
#define LCD_A0_Pin GPIO_PIN_3
#define LCD_A0_GPIO_Port GPIOA
#define LCD_CD_Pin GPIO_PIN_4
#define LCD_CD_GPIO_Port GPIOA
#define Rot_A_Pin GPIO_PIN_1
#define Rot_A_GPIO_Port GPIOB
#define Rot_S_Pin GPIO_PIN_10
#define Rot_S_GPIO_Port GPIOB
#define Rot_F_Pin GPIO_PIN_11
#define Rot_F_GPIO_Port GPIOB
#define Row1_Pin GPIO_PIN_12
#define Row1_GPIO_Port GPIOB
#define Row2_Pin GPIO_PIN_13
#define Row2_GPIO_Port GPIOB
#define Row3_Pin GPIO_PIN_14
#define Row3_GPIO_Port GPIOB
#define Row4_Pin GPIO_PIN_15
#define Row4_GPIO_Port GPIOB
#define Rot_X_Pin GPIO_PIN_8
#define Rot_X_GPIO_Port GPIOA
#define Rot_Y_Pin GPIO_PIN_9
#define Rot_Y_GPIO_Port GPIOA
#define Rot_Z_Pin GPIO_PIN_10
#define Rot_Z_GPIO_Port GPIOA
#define ENC_A_Pin GPIO_PIN_15
#define ENC_A_GPIO_Port GPIOA
#define ENC_B_Pin GPIO_PIN_3
#define ENC_B_GPIO_Port GPIOB
#define Col1_Pin GPIO_PIN_5
#define Col1_GPIO_Port GPIOB
#define Col2_Pin GPIO_PIN_6
#define Col2_GPIO_Port GPIOB
#define Col3_Pin GPIO_PIN_7
#define Col3_GPIO_Port GPIOB
#define Col4_Pin GPIO_PIN_8
#define Col4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
