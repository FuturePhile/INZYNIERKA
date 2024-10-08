/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32l4xx_hal.h"

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
void delay(uint32_t iterations);
void reset_buffer();
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USER_BUTTON_Pin GPIO_PIN_13
#define USER_BUTTON_GPIO_Port GPIOC
#define SERVO_0_Pin GPIO_PIN_0
#define SERVO_0_GPIO_Port GPIOA
#define MAG_SWITCH_Pin GPIO_PIN_11
#define MAG_SWITCH_GPIO_Port GPIOB
#define COL_1_Pin GPIO_PIN_12
#define COL_1_GPIO_Port GPIOB
#define COL_1_EXTI_IRQn EXTI15_10_IRQn
#define COL_2_Pin GPIO_PIN_13
#define COL_2_GPIO_Port GPIOB
#define COL_2_EXTI_IRQn EXTI15_10_IRQn
#define COL_3_Pin GPIO_PIN_14
#define COL_3_GPIO_Port GPIOB
#define COL_3_EXTI_IRQn EXTI15_10_IRQn
#define COL_4_Pin GPIO_PIN_15
#define COL_4_GPIO_Port GPIOB
#define COL_4_EXTI_IRQn EXTI15_10_IRQn
#define ROW_1_Pin GPIO_PIN_6
#define ROW_1_GPIO_Port GPIOC
#define ROW_2_Pin GPIO_PIN_7
#define ROW_2_GPIO_Port GPIOC
#define ROW_3_Pin GPIO_PIN_8
#define ROW_3_GPIO_Port GPIOC
#define ROW_4_Pin GPIO_PIN_9
#define ROW_4_GPIO_Port GPIOC
#define MODEM_ENABLE_Pin GPIO_PIN_5
#define MODEM_ENABLE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
