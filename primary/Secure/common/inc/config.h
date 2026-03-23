/*
 * config.h
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_


#include "stdbool.h"

#include "hal.h"
#include "bootloader.h"


/* ---------------------------------------------------------------------------- */
/* General Config */
/* ---------------------------------------------------------------------------- */

#define NUM_BANKS               (2)

/* ---------------------------------------------------------------------------- */
/* Logging Config */
/* ---------------------------------------------------------------------------- */

#define LOG_BASE            (SRAM1_BASE_NS)
#define LOG_BUFFER_SIZE     (128 * 1024) // TODO: Move to config or generate from linker (preferred)

#define UART_LOGGING_ENABLE (UART_LOGGING_CAPABLE && DEBUG && true) /* Enable if available */

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



/* Exported variables */
static const bootloader_config_t bootloader_config = {
    .log_buffer_base       = LOG_BASE,
    .log_buffer_size       = LOG_BUFFER_SIZE,
    .app_erase_ram_on_boot = true,
    .disable_tick_in_app   = false
};


#endif /* INC_CONFIG_H_ */
