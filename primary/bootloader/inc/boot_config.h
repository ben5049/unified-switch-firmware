/*
 * config.h
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_


#include "stm32h573xx.h"


/* ---------------------------------------------------------------------------- */
/* General Config */
/* ---------------------------------------------------------------------------- */

#define NUM_BANKS               (2)

#define DISABLE_S_SYSTICK_IN_NS (false) /* Whether or not to disable the secure systick when in a non-secure context */

#define ENABLE_UART_LOGGING     (true)

/* ---------------------------------------------------------------------------- */
/* Flash Config (must be updated if the linker file is changed) */
/* ---------------------------------------------------------------------------- */

#define FLASH_S_BANK1_BASE_ADDR  (FLASH_BASE_S)
#define FLASH_S_BANK2_BASE_ADDR  (FLASH_BASE_S + FLASH_BANK_SIZE)

#define FLASH_NS_BANK1_BASE_ADDR (FLASH_BASE_NS)
#define FLASH_NS_BANK2_BASE_ADDR (FLASH_BASE_NS + FLASH_BANK_SIZE)

#define FLASH_S_REGION_OFFSET    0x00000000
#define FLASH_S_REGION_SIZE      (152 * 1024)                                      /* In bytes */

#define FLASH_NSC_REGION_OFFSET  (FLASH_S_REGION_OFFSET + FLASH_S_REGION_SIZE)     /* = 0x00026000 */
#define FLASH_NSC_REGION_SIZE    (8 * 1024)                                        /* In bytes */

#define FLASH_NS_REGION_OFFSET   (FLASH_NSC_REGION_OFFSET + FLASH_NSC_REGION_SIZE) /* = 0x00028000 */
#define FLASH_NS_REGION_SIZE     (864 * 1024)                                      /* In bytes */


#endif /* INC_CONFIG_H_ */
