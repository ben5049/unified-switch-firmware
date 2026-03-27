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
extern uint32_t __FLASH_START__;
extern uint32_t __FLASH_END__;
extern uint32_t __FLASH_SIZE__;
extern uint32_t __FLASH_NSC_START__;
extern uint32_t __FLASH_NSC_END__;
extern uint32_t __FLASH_NSC_SIZE__;
extern uint32_t __FLASH2_START__;
extern uint32_t __FLASH2_END__;
extern uint32_t __FLASH2_SIZE__;
extern uint32_t __FLASH_NSC2_START__;
extern uint32_t __FLASH_NSC2_END__;
extern uint32_t __FLASH_NSC2_SIZE__;


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

#define FLASH_S_REGION_START    ((uint32_t) &__FLASH_START__)
#define FLASH_S_REGION_SIZE     ((uint32_t) &__FLASH_SIZE__) /* In bytes */

#define FLASH_NSC_REGION_START  ((uint32_t) &__FLASH_NSC_START__)
#define FLASH_NSC_REGION_SIZE   ((uint32_t) &__FLASH_NSC_SIZE__) /* In bytes */

#define FLASH2_S_REGION_START   ((uint32_t) &__FLASH2_START__)
#define FLASH2_S_REGION_SIZE    ((uint32_t) &__FLASH2_SIZE__) /* In bytes */

#define FLASH_NSC2_REGION_START ((uint32_t) &__FLASH_NSC2_START__)
#define FLASH_NSC2_REGION_SIZE  ((uint32_t) &__FLASH_NSC2_SIZE__) /* In bytes */

#define FLASH_NS_REGION_START   ((((uint32_t) &__FLASH_NSC_END__) - FLASH_BASE_S) + FLASH_BASE_NS)
#define FLASH_NS_REGION_SIZE    (864 * 1024) /* In bytes */

#define FLASH2_NS_REGION_START  ((((uint32_t) &__FLASH_NSC2_END__) - FLASH_BASE_S) + FLASH_BASE_NS)
#define FLASH2_NS_REGION_SIZE   (FLASH_NS_REGION_SIZE) /* In bytes */


#endif /* INC_BOOTLOADER_CONFIG_H_ */
