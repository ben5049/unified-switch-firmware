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

#if defined ( __ICCARM__ )
#  define CMSE_NS_CALL  __cmse_nonsecure_call
#  define CMSE_NS_ENTRY __cmse_nonsecure_entry
#else
#  define CMSE_NS_CALL  __attribute((cmse_nonsecure_call))
#  define CMSE_NS_ENTRY __attribute((cmse_nonsecure_entry))
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* Function pointer declaration in non-secure*/
#if defined ( __ICCARM__ )
typedef void (CMSE_NS_CALL *funcptr)(void);
#else
typedef void CMSE_NS_CALL (*funcptr)(void);
#endif

/* typedef for non-secure callback functions */
typedef funcptr funcptr_NS;

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
#define MODE_3V3_Pin GPIO_PIN_5
#define MODE_3V3_GPIO_Port GPIOI
#define FRAM_HOLD_Pin GPIO_PIN_15
#define FRAM_HOLD_GPIO_Port GPIOG
#define SMI_SEL3_Pin GPIO_PIN_14
#define SMI_SEL3_GPIO_Port GPIOG
#define PHY2_INT_Pin GPIO_PIN_10
#define PHY2_INT_GPIO_Port GPIOG
#define PHY2_INT_EXTI_IRQn EXTI10_IRQn
#define PHY1_INT_Pin GPIO_PIN_7
#define PHY1_INT_GPIO_Port GPIOD
#define PHY1_INT_EXTI_IRQn EXTI7_IRQn
#define PODL_RST_Pin GPIO_PIN_5
#define PODL_RST_GPIO_Port GPIOD
#define PODL_BOOT_Pin GPIO_PIN_3
#define PODL_BOOT_GPIO_Port GPIOD
#define FRAM_WP_Pin GPIO_PIN_1
#define FRAM_WP_GPIO_Port GPIOI
#define ST_3V3_AON_Pin GPIO_PIN_1
#define ST_3V3_AON_GPIO_Port GPIOE
#define PHY_CLK_EN_Pin GPIO_PIN_6
#define PHY_CLK_EN_GPIO_Port GPIOD
#define STAT_Pin GPIO_PIN_13
#define STAT_GPIO_Port GPIOH
#define PWR_EN_Pin GPIO_PIN_0
#define PWR_EN_GPIO_Port GPIOE
#define FRAM_CS_Pin GPIO_PIN_15
#define FRAM_CS_GPIO_Port GPIOA
#define SWCH_CS1_Pin GPIO_PIN_12
#define SWCH_CS1_GPIO_Port GPIOA
#define SWCH_CS0_Pin GPIO_PIN_11
#define SWCH_CS0_GPIO_Port GPIOA
#define PHY_WAKE_Pin GPIO_PIN_0
#define PHY_WAKE_GPIO_Port GPIOF
#define PHY6_INT_Pin GPIO_PIN_3
#define PHY6_INT_GPIO_Port GPIOF
#define PHY6_INT_EXTI_IRQn EXTI3_IRQn
#define PHY_RST_Pin GPIO_PIN_8
#define PHY_RST_GPIO_Port GPIOF
#define PHY0_INT_Pin GPIO_PIN_2
#define PHY0_INT_GPIO_Port GPIOG
#define PHY0_INT_EXTI_IRQn EXTI2_IRQn
#define PHY4_INT_Pin GPIO_PIN_15
#define PHY4_INT_GPIO_Port GPIOF
#define PHY4_INT_EXTI_IRQn EXTI15_IRQn
#define VIN_VMON_Pin GPIO_PIN_13
#define VIN_VMON_GPIO_Port GPIOF
#define PHY5_INT_Pin GPIO_PIN_1
#define PHY5_INT_GPIO_Port GPIOG
#define PHY5_INT_EXTI_IRQn EXTI1_IRQn
#define SWCH_RST_Pin GPIO_PIN_6
#define SWCH_RST_GPIO_Port GPIOH
#define PHY3_INT_Pin GPIO_PIN_4
#define PHY3_INT_GPIO_Port GPIOH
#define PHY3_INT_EXTI_IRQn EXTI4_IRQn
#define SMI_SEL1_Pin GPIO_PIN_2
#define SMI_SEL1_GPIO_Port GPIOH
#define SMI_SEL2_Pin GPIO_PIN_5
#define SMI_SEL2_GPIO_Port GPIOA
#define VIN_IMON_Pin GPIO_PIN_14
#define VIN_IMON_GPIO_Port GPIOF

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
