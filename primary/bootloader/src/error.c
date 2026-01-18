/*
 * error.c
 *
 *  Created on: Aug 20, 2025
 *      Author: bens1
 */

#include "error.h"
#include "logging.h"
#include "metadata.h"
#include "memory_tools.h"


extern RAMCFG_HandleTypeDef hramcfg_BKPRAM;


void error_handler(error_t major_error_code, uint8_t minor_error_code) {

    __disable_irq();

    /* Turn on the error LED */
    HAL_GPIO_WritePin(STAT_GPIO_Port, STAT_Pin, SET);

    /* NO_CHECK required to prevent recursion */
    LOG_ERROR_NO_CHECK("Error handler :(\n");

    /* Store the most recent logs into the FRAM */
    log_dump_to_fram(&hlog, &hmeta);

    /* Update and store the metadata and counters */
    bool previous_crash    = hmeta.metadata.crashed;
    hmeta.metadata.crashed = true;
    hmeta.counters.crashes++;
    META_dump_metadata(&hmeta);
    META_dump_counters(&hmeta);
    HAL_RAMCFG_Erase(&hramcfg_BKPRAM);

    /* Swap banks to the other bank to try and find a working firmware image. Do not flip-flop between broken images */
    if (!previous_crash) swap_banks();

    while (1) {
    }
}


void nmi_handler() {

    __disable_irq();
    bool source_found = false;

    /* Check if the NMI came from the Clock Security System (CSS) due to HSE failure */
    if (RCC->CIFR & RCC_CIFR_HSECSSF) {
        source_found = true;

        /* Clear the flag by writing 1 */
        RCC->CICR |= RCC_CICR_HSECSSC;

        // Handle clock failure event
        // (e.g., switch to internal oscillator, log fault, reset system, etc.)
        LOG_ERROR("HSE CSS NMI\n");
    }

    /* Check for flash double bit ECC errors */
    if (FLASH->ECCDETR & FLASH_ECCR_ECCD) {
        source_found  = true;
        uint32_t addr = FLASH->ECCDETR & FLASH_ECCDR_FAIL_DATA;
        uint32_t bank = ((FLASH->ECCDETR & FLASH_ECCR_BK_ECC) >> FLASH_ECCR_BK_ECC_Pos) + 1;

        // Optionally parse SYSF_ECC, OTP_ECC, OBK_ECC bits to know the region.

        /* Clear the flag by writing 1 */
        FLASH->ECCDETR |= FLASH_ECCR_ECCD;

        LOG_ERROR("Flash ECC NMI at address 0x%08lx in bank %lu\n", addr, bank);

        /* TODO: Reprogram the bad flash from the other bank */
    }

    /* Check for SRAM double bit ECC errors */
    if ((RAMCFG_SRAM2->ISR & RAMCFG_ISR_DED) || (RAMCFG_SRAM2->ISR & RAMCFG_IER_DEIE) || (RAMCFG_SRAM2->ISR & RAMCFG_IER_ECCNMI)) {
        source_found  = true;
        uint32_t addr = RAMCFG_SRAM2->DEAR;
        LOG_ERROR("SRAM2 ECC NMI at address 0x%08lx\n", addr);
        /* TODO: restart whole chip */
    }
    if ((RAMCFG_SRAM3->ISR & RAMCFG_ISR_DED) || (RAMCFG_SRAM2->ISR & RAMCFG_IER_DEIE) || (RAMCFG_SRAM2->ISR & RAMCFG_IER_ECCNMI)) {
        source_found  = true;
        uint32_t addr = RAMCFG_SRAM3->DEAR;
        LOG_ERROR("SRAM3 ECC NMI at address 0x%08lx\n", addr);
        /* TODO: clear SRAM3 and restart non-secure firmware */
    }
    if ((RAMCFG_BKPRAM->ISR & RAMCFG_ISR_DED) || (RAMCFG_SRAM2->ISR & RAMCFG_IER_DEIE) || (RAMCFG_SRAM2->ISR & RAMCFG_IER_ECCNMI)) {
        source_found  = true;
        uint32_t addr = RAMCFG_BKPRAM->DEAR;
        LOG_ERROR("Backup SRAM ECC NMI at address 0x%08lx\n", addr);
        /* TODO: reload metadata from FRAM if in data or counters regions. Else reset whole chip */
    }

    if (!source_found) {
        LOG_ERROR("Unknown NMI\n");
    }

    while (1) {
    }

    __enable_irq();
}