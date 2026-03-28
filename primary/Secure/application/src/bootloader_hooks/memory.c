/*
 * memory.c
 *
 *  Created on: Mar 26, 2026
 *      Author: bens1
 */

#include "hal.h"
#include "flash.h"
#include "ramcfg.h"

#include "app.h"
#include "bootloader.h"
#include "log_tools.h"


uint8_t bootloader_get_bl_image() {

    /* Since this firmware uses two banks, just get the bank index */
    return (uint8_t) ((FLASH->OPTSR_CUR & FLASH_OPTSR_SWAP_BANK_Msk) >> FLASH_OPTSR_SWAP_BANK_Pos);
}


bootloader_status_t bootloader_program_flash(uint32_t to_addr, uint32_t from_addr, uint32_t size, bool secure, uint32_t timeout) {

    bootloader_status_t status = BL_OK;

    /* Check quad-word alignment and bounds */
    if (from_addr % 16) status = BL_ALIGNMENT_ERROR;
    if (to_addr % 16) status = BL_ALIGNMENT_ERROR;
    if (size % 16) status = BL_ALIGNMENT_ERROR;
    if (secure) {
        if ((to_addr < FLASH_BASE_S) || (to_addr >= (FLASH_BASE_S + FLASH_SIZE_DEFAULT))) status = BL_OOB_ERROR;
        if ((from_addr < FLASH_BASE_S) || (from_addr >= (FLASH_BASE_S + FLASH_SIZE_DEFAULT))) status = BL_OOB_ERROR;
    } else {
        if ((to_addr < FLASH_BASE_NS) || (to_addr >= (FLASH_BASE_NS + FLASH_SIZE_DEFAULT))) status = BL_OOB_ERROR;
        if ((from_addr < FLASH_BASE_NS) || (from_addr >= (FLASH_BASE_NS + FLASH_SIZE_DEFAULT))) status = BL_OOB_ERROR;
    }
    if (status != BL_OK) return status;

    /* Don't program during debug TODO: this needs to be tested */
#if !DEBUG

    /* Unlock the flash */
    LOG_INFO("Unlocking flash");
    HAL_FLASH_Unlock();

    /* Write the data */
    if (LOG_INFO_NO_CHECK("Writing %luKiB to flash address 0x%08lx", size >> 10, to_addr) != LOG_OK) goto end; /* NO_CHECK since flash must be locked */
    for (uint_fast32_t offset = 0; offset < size; offset += 16) {

        /* Program the quadword */
        if (HAL_FLASH_Program(secure ? FLASH_TYPEPROGRAM_QUADWORD : FLASH_TYPEPROGRAM_QUADWORD_NS, to_addr + offset, from_addr + offset) != HAL_OK) {
            status = BL_MEM_PROGRAM_ERROR;
            LOG_ERROR_NO_CHECK("Failed to write to flash address 0x%08lx\n", to_addr + offset);
            goto end;
        }
    }

/* Lock the flash */
end:
    HAL_FLASH_Lock();
    LOG_INFO("Locking flash\n");

#endif

    return status;
}


bootloader_status_t bootloader_erase_ram(bootloader_erase_ram_t erase) {

    hal_status_t status = BL_OK;

    switch (erase) {

        case BL_ERASE_RAM_ALL:
            status = HAL_RAMCFG_Erase(&hramcfg_SRAM1);
            if (status != HAL_OK) return BL_ERROR;
            /* Note there is no SRAM2 since that would mean the bootloader erasing itself */
            status = HAL_RAMCFG_Erase(&hramcfg_SRAM3);
            if (status != HAL_OK) return BL_ERROR;
            status = HAL_RAMCFG_Erase(&hramcfg_BKPRAM);
            break;

        /* Hardware erase the log buffer RAM */
        case BL_ERASE_RAM_LOG:
            status = HAL_RAMCFG_Erase(&hramcfg_SRAM1);
            break;

        /* Hardware erase the non-secure applications's RAM */
        case BL_ERASE_RAM_APP:
            status = HAL_RAMCFG_Erase(&hramcfg_SRAM3);
            break;

        case BL_ERASE_RAM_META:
            status = HAL_RAMCFG_Erase(&hramcfg_BKPRAM);
            break;

        default:
            status = HAL_ERROR;
            break;
    }

    return (status == HAL_OK) ? BL_OK : BL_ERROR;
}


void bootloader_jump_to_bootloader(uint8_t image) {

    if (image != bootloader_get_bl_image()) {
        // TODO: swap banks and reboot
    }

    /* In debug mode don't actually jump */
#if DEBUG
    while (1);
#endif

    HAL_NVIC_SystemReset();
}


void bootloader_jump_to_application(uint8_t image) {

    if (image != bootloader_get_bl_image()) {
        // TODO: swap banks and reboot
    }

    /* Shouldn't return */
    nonsecure_init();
}
