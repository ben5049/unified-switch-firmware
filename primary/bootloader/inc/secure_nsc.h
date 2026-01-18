/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    Secure_nsclib/secure_nsc.h
 * @author  MCD Application Team
 * @brief   Header for secure non-secure callable APIs list
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

/* USER CODE BEGIN Non_Secure_CallLib_h */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SECURE_NSC_H
#define SECURE_NSC_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "nxd_dhcp_client.h"


void s_save_dhcp_client_record(const NX_DHCP_CLIENT_RECORD *record);
void s_load_dhcp_client_record(NX_DHCP_CLIENT_RECORD *record);
void s_background_task(void);
void s_log_vwrite(const char *format, va_list args);


#endif /* SECURE_NSC_H */
/* USER CODE END Non_Secure_CallLib_h */
