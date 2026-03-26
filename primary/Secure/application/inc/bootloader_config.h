/*
 * bootloader_config.h
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#ifndef INC_BOOTLOADER_CONFIG_H_
#define INC_BOOTLOADER_CONFIG_H_


#include "stdbool.h"

#include "hal.h"
#include "bootloader.h"


/* Imported linker symbols */
extern uint32_t __LOG_NS_START__;
extern uint32_t __LOG_NS_SIZE__;


/* ---------------------------------------------------------------------------- */
/* General Config */
/* ---------------------------------------------------------------------------- */

#define NUM_BANKS (2)

/* ---------------------------------------------------------------------------- */
/* Logging Config */
/* ---------------------------------------------------------------------------- */

#define LOG_BASE            ((uint32_t) &__LOG_NS_START__)
#define LOG_BUFFER_SIZE     ((uint32_t) &__LOG_NS_SIZE__)

#define UART_LOGGING_ENABLE (DEBUG != 0)

/* ---------------------------------------------------------------------------- */
/* Flash Config (must be updated if the linker file is changed) */
/* ---------------------------------------------------------------------------- */

#define FLASH_S_BANK1_BASE_ADDR  (FLASH_BASE_S)
#define FLASH_S_BANK2_BASE_ADDR  (FLASH_BASE_S + (FLASH_SIZE_DEFAULT >> 1))

#define FLASH_NS_BANK1_BASE_ADDR (FLASH_BASE_NS)
#define FLASH_NS_BANK2_BASE_ADDR (FLASH_BASE_NS + (FLASH_SIZE_DEFAULT >> 1))

#define FLASH_S_REGION_OFFSET    (0x00000000)
#define FLASH_S_REGION_SIZE      (152 * 1024)                                      /* In bytes */

#define FLASH_NSC_REGION_OFFSET  (FLASH_S_REGION_OFFSET + FLASH_S_REGION_SIZE)     /* = 0x00026000 */
#define FLASH_NSC_REGION_SIZE    (8 * 1024)                                        /* In bytes */

#define FLASH_NS_REGION_OFFSET   (FLASH_NSC_REGION_OFFSET + FLASH_NSC_REGION_SIZE) /* = 0x00028000 */
#define FLASH_NS_REGION_SIZE     (864 * 1024)                                      /* In bytes */


#endif /* INC_BOOTLOADER_CONFIG_H_ */
