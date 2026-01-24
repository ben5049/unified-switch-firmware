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
     PA14(JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
     PC15-OSC32_OUT(OSC32_OUT)   ------> RCC_OSC32_OUT
     PC14-OSC32_IN(OSC32_IN)   ------> RCC_OSC32_IN
     PA13(JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
     PH0-OSC_IN(PH0)   ------> RCC_OSC_IN
     PH1-OSC_OUT(PH1)   ------> RCC_OSC_OUT
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FRAM_HOLD_GPIO_Port, FRAM_HOLD_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, PODL_RST_Pin|PODL_BOOT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FRAM_WP_GPIO_Port, FRAM_WP_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(STAT_GPIO_Port, STAT_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(PWR_EN_GPIO_Port, PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FRAM_CS_GPIO_Port, FRAM_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PI7 PI4 PI6 PI9
                           PI8 PI11 PI10 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_4|GPIO_PIN_6|GPIO_PIN_9
                          |GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pins : PB9 PB8 PB6 PB7
                           PB10 PB1 PB14 PB2
                           PB0 PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_8|GPIO_PIN_6|GPIO_PIN_7
                          |GPIO_PIN_10|GPIO_PIN_1|GPIO_PIN_14|GPIO_PIN_2
                          |GPIO_PIN_0|GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : FRAM_HOLD_Pin */
  GPIO_InitStruct.Pin = FRAM_HOLD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FRAM_HOLD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PODL_RST_Pin PODL_BOOT_Pin */
  GPIO_InitStruct.Pin = PODL_RST_Pin|PODL_BOOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : FRAM_WP_Pin */
  GPIO_InitStruct.Pin = FRAM_WP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FRAM_WP_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PE3 PE6 PE4 PE5
                           PE2 PE9 PE14 PE15
                           PE8 PE11 PE13 PE7
                           PE10 PE12 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_6|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_2|GPIO_PIN_9|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_13|GPIO_PIN_7
                          |GPIO_PIN_10|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PG12 PG9 PG13 PG11
                           PG8 PG7 PG3 PG5
                           PG6 PG4 PG0 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_9|GPIO_PIN_13|GPIO_PIN_11
                          |GPIO_PIN_8|GPIO_PIN_7|GPIO_PIN_3|GPIO_PIN_5
                          |GPIO_PIN_6|GPIO_PIN_4|GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : PD4 PD2 PD15 PD10
                           PD14 PD12 PD9 PD11
                           PD13 PD8 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_2|GPIO_PIN_15|GPIO_PIN_10
                          |GPIO_PIN_14|GPIO_PIN_12|GPIO_PIN_9|GPIO_PIN_11
                          |GPIO_PIN_13|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : STAT_Pin */
  GPIO_InitStruct.Pin = STAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(STAT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PWR_EN_Pin */
  GPIO_InitStruct.Pin = PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(PWR_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC12 PC11 PC10 PC13
                           PC9 PC8 PC7 PC6
                           PC2 PC0 PC3 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_11|GPIO_PIN_10|GPIO_PIN_13
                          |GPIO_PIN_9|GPIO_PIN_8|GPIO_PIN_7|GPIO_PIN_6
                          |GPIO_PIN_2|GPIO_PIN_0|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : FRAM_CS_Pin */
  GPIO_InitStruct.Pin = FRAM_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(FRAM_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PH15 PH14 PH9 PH12
                           PH8 PH10 PH5 PH3
                           PH7 PH11 */
  GPIO_InitStruct.Pin = GPIO_PIN_15|GPIO_PIN_14|GPIO_PIN_9|GPIO_PIN_12
                          |GPIO_PIN_8|GPIO_PIN_10|GPIO_PIN_5|GPIO_PIN_3
                          |GPIO_PIN_7|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pins : PA10 PA8 PA4 PA3
                           PA0 PA6 */
  GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_8|GPIO_PIN_4|GPIO_PIN_3
                          |GPIO_PIN_0|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PF1 PF4 PF2 PF6
                           PF5 PF9 PF10 PF7
                           PF12 PF11 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_2|GPIO_PIN_6
                          |GPIO_PIN_5|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_7
                          |GPIO_PIN_12|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOI, MODE_3V3_Pin|GPIO_PIN_3|GPIO_PIN_2, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOG, SMI_SEL3_Pin|PHY2_INT_Pin|PHY0_INT_Pin|PHY5_INT_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOD, PHY1_INT_Pin|PHY_CLK_EN_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(ST_3V3_AON_GPIO_Port, ST_3V3_AON_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOA, SWCH_CS1_Pin|SWCH_CS0_Pin|GPIO_PIN_9|GPIO_PIN_1
                          |GPIO_PIN_2|SMI_SEL2_Pin|GPIO_PIN_7, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOF, PHY_WAKE_Pin|PHY6_INT_Pin|PHY_RST_Pin|PHY4_INT_Pin
                          |VIN_VMON_Pin|VIN_IMON_Pin, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_1|GPIO_PIN_4|GPIO_PIN_5, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_12|GPIO_PIN_15|GPIO_PIN_11, GPIO_PIN_NSEC);

  /*IO attributes management functions */
  HAL_GPIO_ConfigPinAttributes(GPIOH, SWCH_RST_Pin|PHY3_INT_Pin|SMI_SEL1_Pin, GPIO_PIN_NSEC);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
