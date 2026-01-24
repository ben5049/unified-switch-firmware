/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PHY_CLK_EN_GPIO_Port, PHY_CLK_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SWCH_CS1_Pin|SWCH_CS0_Pin|SMI_SEL2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, PHY_WAKE_Pin|PHY_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOH, SWCH_RST_Pin|SMI_SEL1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : MODE_3V3_Pin */
  GPIO_InitStruct.Pin = MODE_3V3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MODE_3V3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SMI_SEL3_Pin */
  GPIO_InitStruct.Pin = SMI_SEL3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SMI_SEL3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PHY2_INT_Pin PHY0_INT_Pin PHY5_INT_Pin */
  GPIO_InitStruct.Pin = PHY2_INT_Pin|PHY0_INT_Pin|PHY5_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : PHY1_INT_Pin */
  GPIO_InitStruct.Pin = PHY1_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(PHY1_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ST_3V3_AON_Pin */
  GPIO_InitStruct.Pin = ST_3V3_AON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ST_3V3_AON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PHY_CLK_EN_Pin */
  GPIO_InitStruct.Pin = PHY_CLK_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PHY_CLK_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SWCH_CS1_Pin SWCH_CS0_Pin */
  GPIO_InitStruct.Pin = SWCH_CS1_Pin|SWCH_CS0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PHY_WAKE_Pin */
  GPIO_InitStruct.Pin = PHY_WAKE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PHY_WAKE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PHY6_INT_Pin PHY4_INT_Pin */
  GPIO_InitStruct.Pin = PHY6_INT_Pin|PHY4_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pin : PHY_RST_Pin */
  GPIO_InitStruct.Pin = PHY_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PHY_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SWCH_RST_Pin */
  GPIO_InitStruct.Pin = SWCH_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SWCH_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PHY3_INT_Pin */
  GPIO_InitStruct.Pin = PHY3_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(PHY3_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SMI_SEL1_Pin */
  GPIO_InitStruct.Pin = SMI_SEL1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SMI_SEL1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SMI_SEL2_Pin */
  GPIO_InitStruct.Pin = SMI_SEL2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SMI_SEL2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI7_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI7_IRQn);

  HAL_NVIC_SetPriority(EXTI10_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI10_IRQn);

  HAL_NVIC_SetPriority(EXTI15_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(EXTI15_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
