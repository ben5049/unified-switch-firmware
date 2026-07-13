/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32h5xx_hal.h"

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
#define PODL2_DET_EN_Pin GPIO_PIN_14
#define PODL2_DET_EN_GPIO_Port GPIOC
#define VIN_IMON_Pin GPIO_PIN_0
#define VIN_IMON_GPIO_Port GPIOA
#define PWR_2V5_EN_Pin GPIO_PIN_1
#define PWR_2V5_EN_GPIO_Port GPIOA
#define VIN_VMON_Pin GPIO_PIN_2
#define VIN_VMON_GPIO_Port GPIOA
#define PODL1_IMON_Pin GPIO_PIN_3
#define PODL1_IMON_GPIO_Port GPIOA
#define PODL1_DET_VP_Pin GPIO_PIN_4
#define PODL1_DET_VP_GPIO_Port GPIOA
#define PODL3_IMON_Pin GPIO_PIN_6
#define PODL3_IMON_GPIO_Port GPIOA
#define PODL3_IMONA7_Pin GPIO_PIN_7
#define PODL3_IMONA7_GPIO_Port GPIOA
#define PODL3_DET_VP_Pin GPIO_PIN_0
#define PODL3_DET_VP_GPIO_Port GPIOB
#define PODL2_DET_VP_Pin GPIO_PIN_1
#define PODL2_DET_VP_GPIO_Port GPIOB
#define PODL1_EN_Pin GPIO_PIN_8
#define PODL1_EN_GPIO_Port GPIOA
#define PODL1_DET_EN_Pin GPIO_PIN_9
#define PODL1_DET_EN_GPIO_Port GPIOA
#define PODL_DIAG_EN_Pin GPIO_PIN_11
#define PODL_DIAG_EN_GPIO_Port GPIOA
#define PODL2_EN_Pin GPIO_PIN_12
#define PODL2_EN_GPIO_Port GPIOA
#define MODE_3V3_Pin GPIO_PIN_5
#define MODE_3V3_GPIO_Port GPIOB
#define PODL3_DET_EN_Pin GPIO_PIN_6
#define PODL3_DET_EN_GPIO_Port GPIOB
#define PODL3_EN_Pin GPIO_PIN_7
#define PODL3_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
