/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    aes.c
  * @brief   This file provides code for the configuration
  *          of the AES instances.
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
#include "aes.h"

/* USER CODE BEGIN 0 */
#include "secrets.h"
/* USER CODE END 0 */

CRYP_HandleTypeDef hcryp;
uint32_t pKeySAES[4] = {0xD3F7C3E5,0x8C577EAC,0x766ECACB,0x5844BC45};
uint32_t pInitVectSAES[4] = {0x020E71AF,0x8748FC25,0x9CD1C0F7,0x9ACECBDA};

/* SAES init function */

void MX_SAES_AES_Init(void)
{

  /* USER CODE BEGIN SAES_Init 0 */
  set_saes_key(pKeySAES);
  set_saes_init_vector(pInitVectSAES);
  /* USER CODE END SAES_Init 0 */

  /* USER CODE BEGIN SAES_Init 1 */

  /* USER CODE END SAES_Init 1 */
  hcryp.Instance = SAES;
  hcryp.Init.DataType = CRYP_NO_SWAP;
  hcryp.Init.KeySize = CRYP_KEYSIZE_128B;
  hcryp.Init.pKey = (uint32_t *)pKeySAES;
  hcryp.Init.pInitVect = (uint32_t *)pInitVectSAES;
  hcryp.Init.Algorithm = CRYP_AES_CTR;
  hcryp.Init.DataWidthUnit = CRYP_DATAWIDTHUNIT_BYTE;
  hcryp.Init.HeaderWidthUnit = CRYP_HEADERWIDTHUNIT_BYTE;
  hcryp.Init.KeyIVConfigSkip = CRYP_KEYIVCONFIG_ALWAYS;
  hcryp.Init.KeyMode = CRYP_KEYMODE_NORMAL;
  hcryp.Init.KeySelect = CRYP_KEYSEL_NORMAL;
  hcryp.Init.KeyProtection = CRYP_KEYPROT_DISABLE;
  if (HAL_CRYP_Init(&hcryp) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAES_Init 2 */

  /* USER CODE END SAES_Init 2 */

}

void HAL_CRYP_MspInit(CRYP_HandleTypeDef* crypHandle)
{

  if(crypHandle->Instance==SAES)
  {
  /* USER CODE BEGIN SAES_MspInit 0 */

  /* USER CODE END SAES_MspInit 0 */
    /* SAES clock enable */
    __HAL_RCC_SAES_CLK_ENABLE();

    /* SAES interrupt Init */
    HAL_NVIC_SetPriority(SAES_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(SAES_IRQn);
  /* USER CODE BEGIN SAES_MspInit 1 */

  /* USER CODE END SAES_MspInit 1 */
  }
}

void HAL_CRYP_MspDeInit(CRYP_HandleTypeDef* crypHandle)
{

  if(crypHandle->Instance==SAES)
  {
  /* USER CODE BEGIN SAES_MspDeInit 0 */

  /* USER CODE END SAES_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_SAES_CLK_DISABLE();

    /* SAES interrupt Deinit */
    HAL_NVIC_DisableIRQ(SAES_IRQn);
  /* USER CODE BEGIN SAES_MspDeInit 1 */

  /* USER CODE END SAES_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

