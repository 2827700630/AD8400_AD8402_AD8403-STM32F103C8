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
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define AD840X_CS1_Pin GPIO_PIN_0
#define AD840X_CS1_GPIO_Port GPIOB
#define AD840X_RS1_Pin GPIO_PIN_1
#define AD840X_RS1_GPIO_Port GPIOB
#define AD840X_SHDN1_Pin GPIO_PIN_10
#define AD840X_SHDN1_GPIO_Port GPIOB
#define AD840X_CS2_Pin GPIO_PIN_11
#define AD840X_CS2_GPIO_Port GPIOB
#define AD840X_RS2_Pin GPIO_PIN_12
#define AD840X_RS2_GPIO_Port GPIOB
#define AD840X_SHDN2_Pin GPIO_PIN_13
#define AD840X_SHDN2_GPIO_Port GPIOB
#define AD840X_CS3_Pin GPIO_PIN_14
#define AD840X_CS3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
