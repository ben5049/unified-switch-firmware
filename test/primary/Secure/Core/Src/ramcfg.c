/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    ramcfg.c
  * @brief   This file provides code for the configuration
  *          of the RAMCFG instances.
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
/* Includes ------------------------------------------------------------------*/
#include "ramcfg.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

RAMCFG_HandleTypeDef hramcfg_SRAM1;
RAMCFG_HandleTypeDef hramcfg_SRAM2;
RAMCFG_HandleTypeDef hramcfg_SRAM3;
RAMCFG_HandleTypeDef hramcfg_BKPRAM;

/* RAMCFG init function */
void MX_RAMCFG_Init(void)
{

  /* USER CODE BEGIN RAMCFG_Init 0 */

  /* USER CODE END RAMCFG_Init 0 */

  /* USER CODE BEGIN RAMCFG_Init 1 */

  /* USER CODE END RAMCFG_Init 1 */

  /** Initialize RAMCFG SRAM1
  */
  hramcfg_SRAM1.Instance = RAMCFG_SRAM1;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initialize RAMCFG SRAM2
  */
  hramcfg_SRAM2.Instance = RAMCFG_SRAM2;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_RAMCFG_StartECC(&hramcfg_SRAM2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initialize RAMCFG SRAM3
  */
  hramcfg_SRAM3.Instance = RAMCFG_SRAM3;
  if (HAL_RAMCFG_Init(&hramcfg_SRAM3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_RAMCFG_StartECC(&hramcfg_SRAM3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initialize RAMCFG BKPRAM
  */
  hramcfg_BKPRAM.Instance = RAMCFG_BKPRAM;
  if (HAL_RAMCFG_Init(&hramcfg_BKPRAM) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_RAMCFG_StartECC(&hramcfg_BKPRAM) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RAMCFG_Init 2 */

  /* USER CODE END RAMCFG_Init 2 */

}

void HAL_RAMCFG_MspInit(RAMCFG_HandleTypeDef* ramcfgHandle)
{

  /* USER CODE BEGIN RAMCFG_MspInit 0 */

  /* USER CODE END RAMCFG_MspInit 0 */
    /* RAMCFG clock enable */
    __HAL_RCC_RAMCFG_CLK_ENABLE();

    /* RAMCFG interrupt Init */
    HAL_NVIC_SetPriority(RAMCFG_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RAMCFG_IRQn);
  /* USER CODE BEGIN RAMCFG_MspInit 1 */

  /* USER CODE END RAMCFG_MspInit 1 */
}

void HAL_RAMCFG_MspDeInit(RAMCFG_HandleTypeDef* ramcfgHandle)
{

  /* USER CODE BEGIN RAMCFG_MspDeInit 0 */

  /* USER CODE END RAMCFG_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RAMCFG_CLK_DISABLE();

    /* RAMCFG interrupt Deinit */
    HAL_NVIC_DisableIRQ(RAMCFG_IRQn);
  /* USER CODE BEGIN RAMCFG_MspDeInit 1 */

  /* USER CODE END RAMCFG_MspDeInit 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
