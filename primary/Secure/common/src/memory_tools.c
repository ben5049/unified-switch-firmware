/*
 * memory_tools.c
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "memory.h"
#include "hal.h"
#include "main.h"

#include "memory_tools.h"
#include "config.h"
#include "logging.h"
#include "integrity.h"
#include "metadata.h"
#include "log_tools.h"


static uint32_t ns_firmware_write_addr = 0;


bool get_bank_swap() {
    FLASH_OBProgramInitTypeDef ob;
    HAL_FLASHEx_OBGetConfig(&ob);
    return ob.USERConfig & FLASH_OPTSR_SWAP_BANK;
}


void swap_banks() {
    // should also hardware erase the backup sram
    /* In debug mode don't actually swap banks */

    // TODO: WRITE THIS
#if DEBUG
    __BKPT(0);
#endif

    HAL_NVIC_SystemReset();
}


/* Note this function is called before logging is started */
void enable_backup_domain(void) {

    /* Enable write access to backup domain */
    PWR_S->DBPCR |= PWR_DBPCR_DBP; /* Disable write protection */

    /* Enable clock to backup domain (updated __HAL_RCC_BKPRAM_CLK_ENABLE() for secure world) */
    __IO uint32_t tmpreg;
    SET_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);
    /* Delay after an RCC peripheral clock enabling */
    tmpreg = READ_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);
    UNUSED(tmpreg);

    /* Enable retention through sleep and power-off */
    PWR_S->BDCR |= PWR_BDCR_BREN;

    /* Enable voltage and temperature monitoring. TODO: Use this? */
    // PWR_S->BDCR  |= PWR_BDCR_MONEN;
}


/* Check secure firmware */
bool check_s_firmware(uint8_t bank) {

    uint8_t status = 0;
    bool    valid  = true;

    /* Make sure the valid flag is set */
    if (bank == FLASH_BANK_1) {
        valid = hmeta.metadata.s_firmware_1_valid;
    } else if (bank == FLASH_BANK_2) {
        valid = hmeta.metadata.s_firmware_2_valid;
    } else {
        valid = false;
    }
    if (!valid) return valid;

    /* Make sure secure firmwares are supposed to be identical */
    valid = memcmp(hmeta.metadata.s_firmware_1_hash, hmeta.metadata.s_firmware_2_hash, SHA256_SIZE) == 0;
    if (!valid) return valid;

    /* Compute the hash */
    status = INTEGRITY_compute_s_firmware_hash(bank);
    CHECK_STATUS_INTEGRITY(status);

    /* Compare the computed hash with the stored hash */
    status = META_check_s_firmware_hash(&hmeta, bank, INTEGRITY_get_s_firmware_hash(bank), &valid);
    CHECK_STATUS_META(status);

    /* Update hmeta */
    if (bank == FLASH_BANK_1) {
        hmeta.metadata.s_firmware_1_valid = valid;
    } else if (bank == FLASH_BANK_2) {
        hmeta.metadata.s_firmware_2_valid = valid;
    }

    return valid;
}


/* Non-secure firmware check */
bool check_ns_firmware(uint8_t bank) {

    uint8_t status = 0;
    bool    valid  = true;

    /* Make sure the valid flag is set */
    if (bank == FLASH_BANK_1) {
        valid = hmeta.metadata.ns_firmware_1_valid;
    } else if (bank == FLASH_BANK_2) {
        valid = hmeta.metadata.ns_firmware_2_valid;
    } else {
        valid = false;
    }
    if (!valid) return valid;

    /* Compute the hash */
    status = INTEGRITY_compute_ns_firmware_hash(bank);
    CHECK_STATUS_INTEGRITY(status);

    /* Compare the computed hash with the stored hash */
    status = META_check_ns_firmware_hash(&hmeta, bank, INTEGRITY_get_ns_firmware_hash(bank), &valid);
    CHECK_STATUS_META(status);

    /* Update hmeta */
    if (bank == FLASH_BANK_1) {
        hmeta.metadata.ns_firmware_1_valid = valid;
    } else if (bank == FLASH_BANK_2) {
        hmeta.metadata.ns_firmware_2_valid = valid;
    }

    return valid;
}


static memory_status_t write_flash(uint32_t addr, uint8_t *data, uint32_t size, bool secure) {

    memory_status_t status = MEM_OK;

    /* Check the inputs */
    if (addr % 16 != 0) status = MEM_ALIGNMENT_ERROR;           /* Address must be quadword aligned */
    if (size % 16 != 0) status = MEM_ALIGNMENT_ERROR;           /* Size must be divisible by quadwords */
    if (data == NULL) status = MEM_PARAMETER_ERROR;             /* No data to write */
    if ((uint32_t) data % 4 != 0) status = MEM_ALIGNMENT_ERROR; /* Data must be word aligned */
    if (status != MEM_OK) return status;

    /* Bounds checking */
    if (secure) {

        /* Make sure the start address is within the S flash region. Note that it isn't possible to write to the BANK1 secure flash since that is where we are currently executing from */
        if (!((addr >= (FLASH_S_BANK2_BASE_ADDR + FLASH_S_REGION_OFFSET)) && (addr < (FLASH_S_BANK2_BASE_ADDR + FLASH_S_REGION_OFFSET + FLASH_S_REGION_SIZE)))) status = MEM_INVALID_ADDRESS_ERROR;
        if (status != MEM_OK) return status;

        /* Make sure the end address is within the S flash region. Note that it isn't possible to write to the BANK1 secure flash since that is where we are currently executing from */
        if (!((addr + size > (FLASH_S_BANK2_BASE_ADDR + FLASH_S_REGION_OFFSET)) && (addr + size <= (FLASH_S_BANK2_BASE_ADDR + FLASH_S_REGION_OFFSET + FLASH_S_REGION_SIZE)))) status = MEM_INVALID_ADDRESS_ERROR;
        if (status != MEM_OK) return status;

    } else {

        /* Make sure the start address is within the NS flash region */
        if (!((addr >= (FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET)) && (addr < (FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET + FLASH_NS_REGION_SIZE))) &&
            !((addr >= (FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET)) && (addr < (FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET + FLASH_NS_REGION_SIZE)))) status = MEM_INVALID_ADDRESS_ERROR;
        if (status != MEM_OK) return status;

        /* Make sure the end address is within the NS flash region */
        if (!((addr + size > (FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET)) && (addr + size <= (FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET + FLASH_NS_REGION_SIZE))) &&
            !((addr + size > (FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET)) && (addr + size <= (FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET + FLASH_NS_REGION_SIZE)))) status = MEM_INVALID_ADDRESS_ERROR;
        if (status != MEM_OK) return status;
    }

    /* Unlock the flash */
    LOG_INFO("Unlocking flash\n");
    HAL_FLASH_Unlock();

    /* Write the data */
    LOG_INFO_NO_CHECK("Writing %lu bytes to flash address 0x%08lx\n", size, addr); /* NO_CHECK since flash must be locked */
    for (uint_fast32_t i = 0; i < size; i += 16) {

        /* Program the quadword */
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, addr + i, (uint32_t) (data + i)) != HAL_OK) {
            status = MEM_PROGRAM_ERROR;
            LOG_ERROR_NO_CHECK("Failed to write to flash address 0x%08lx\n", addr + i);
            goto end;
        }
    }

    /* Lock the flash */
end:
    HAL_FLASH_Lock();
    LOG_INFO("Locking flash\n");
    return status;
}


memory_status_t copy_s_firmware_to_other_bank(bool bank_swap) {

    memory_status_t status = MEM_OK;
    bool            valid  = true;

    /* Don't write when debugging */
#if DEBUG
    return status;
#endif

    _Static_assert(((FLASH_S_BANK1_BASE_ADDR + FLASH_S_REGION_OFFSET) % 16) == 0, "Not quad-word aligned");
    _Static_assert(((FLASH_S_BANK2_BASE_ADDR + FLASH_S_REGION_OFFSET) % 16) == 0, "Not quad-word aligned");
    _Static_assert((FLASH_S_REGION_SIZE % 16) == 0, "Not quad-word aligned");

    if (!bank_swap) {

        /* Set invalid before copying */
        hmeta.metadata.s_firmware_2_valid = false;

        /* Do the copy (note we execute from whichever bank is at 0x0c000000 so the other secure firmware is always at 0x0c100000) */
        status = write_flash(FLASH_S_BANK2_BASE_ADDR + FLASH_S_REGION_OFFSET, (uint8_t *) (FLASH_S_BANK1_BASE_ADDR + FLASH_S_REGION_OFFSET), FLASH_S_REGION_SIZE, true);
        if (status != MEM_OK) return status;

        /* Set what the hash should be */
        if (META_set_s_firmware_hash(&hmeta, FLASH_BANK_2, INTEGRITY_get_s_firmware_hash(FLASH_BANK_1)) != META_OK) {
            status = MEM_SET_HASH_ERROR;
            return status;
        }

        /* Check the hash and return */
        valid = check_s_firmware(FLASH_BANK_2);
        if (!valid) status = MEM_POST_WRITE_CHECK_ERROR;
    } else {

        /* Set invalid before copying */
        hmeta.metadata.s_firmware_1_valid = false;

        /* Do the copy (note we execute from whichever bank is at 0x0c000000 so the other secure firmware is always at 0x0c100000) */
        status = write_flash(FLASH_S_BANK2_BASE_ADDR + FLASH_S_REGION_OFFSET, (uint8_t *) (FLASH_S_BANK1_BASE_ADDR + FLASH_S_REGION_OFFSET), FLASH_S_REGION_SIZE, true);
        if (status != MEM_OK) return status;

        /* Set what the hash should be */
        if (META_set_s_firmware_hash(&hmeta, FLASH_BANK_1, INTEGRITY_get_s_firmware_hash(FLASH_BANK_2)) != META_OK) {
            status = MEM_SET_HASH_ERROR;
            return status;
        }

        /* Check the hash and return */
        valid = check_s_firmware(FLASH_BANK_1);
        if (!valid) status = MEM_POST_WRITE_CHECK_ERROR;
    }

    return status;
}


memory_status_t copy_ns_firmware_to_other_bank(bool bank_swap) {
    if (!bank_swap) {
        return copy_ns_firmware(FLASH_BANK_1, FLASH_BANK_2);
    } else {
        return copy_ns_firmware(FLASH_BANK_2, FLASH_BANK_1);
    }
}


memory_status_t copy_ns_firmware(uint8_t from_bank, uint8_t to_bank) {

    memory_status_t status = MEM_OK;
    bool            valid  = true;

    /* Don't write when debugging */
#if DEBUG
    return status;
#endif

    /* Check memory alignment */
    _Static_assert(((FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET) % 16) == 0, "Not quad-word aligned");
    _Static_assert(((FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET) % 16) == 0, "Not quad-word aligned");
    _Static_assert((FLASH_NS_REGION_SIZE % 16) == 0, "Not quad-word aligned");

    /* Turn banks numbers into addresses */
    uint8_t *from_ptr;
    uint32_t to_addr;
    if (((!hmeta.bank_swap) && ((from_bank == FLASH_BANK_1) && (to_bank == FLASH_BANK_2))) ||
        ((hmeta.bank_swap) && ((from_bank == FLASH_BANK_2) && (to_bank == FLASH_BANK_1)))) {
        from_ptr = (uint8_t *) (FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET);
        to_addr  = FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET;
    } else if (((!hmeta.bank_swap) && ((from_bank == FLASH_BANK_2) && (to_bank == FLASH_BANK_1))) ||
               ((hmeta.bank_swap) && ((from_bank == FLASH_BANK_1) && (to_bank == FLASH_BANK_2)))) {
        from_ptr = (uint8_t *) (FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET);
        to_addr  = FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET;
    } else {
        status = MEM_PARAMETER_ERROR;
        return status;
    }

    /* Set invalid before copying */
    if (to_bank == FLASH_BANK_1) {
        hmeta.metadata.ns_firmware_1_valid = false;
    } else {
        hmeta.metadata.ns_firmware_2_valid = false;
    }

    /* Do the copy */
    status = write_flash(to_addr, from_ptr, FLASH_NS_REGION_SIZE, false);
    if (status != MEM_OK) return status;

    /* Set what the hash should be */
    if (META_set_ns_firmware_hash(&hmeta, to_bank, INTEGRITY_get_ns_firmware_hash(from_bank)) != META_OK) {
        status = MEM_SET_HASH_ERROR;
        return status;
    }

    /* Check the hash and return (also updates valid) */
    valid = check_ns_firmware(to_bank);
    if (!valid) status = MEM_POST_WRITE_CHECK_ERROR;

    return status;
}


/* ---------------------------------------------------------------------------- */
/* Non-secure firmware update */
/* ---------------------------------------------------------------------------- */


memory_status_t ns_firmware_update_start(uint8_t *hash, uint8_t *signature_r, uint8_t *signature_s) {

    memory_status_t status = MEM_OK;
    uint8_t         bank   = OTHER_FLASH_BANK(hmeta.bank_swap);

    /* Check the signature of the firmware */
    if (INTEGRITY_check_firmware_signature(hash, signature_r, signature_s) != INTEGRITY_OK) status = MEM_SIGNATURE_ERROR;
    if (status != MEM_OK) return status;

    /* Signature is valid: invalidate the current image to make space */
    if (!hmeta.bank_swap) {
        hmeta.metadata.ns_firmware_2_valid = false;
    } else {
        hmeta.metadata.ns_firmware_1_valid = false;
    }

    /* Set the expected hash of the firmware (the actual hash will be calculated and checked at the end before setting valid to true) */
    if (META_set_ns_firmware_hash(&hmeta, bank, hash) != META_OK) status = MEM_SET_HASH_ERROR;
    if (status != MEM_OK) return status;

    /* Set the write pointer (we always execute from 0x08000000 so the other bank is always at 0x08100000) */
    ns_firmware_write_addr = FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET;

    return status;
}


memory_status_t ns_firmware_update_write(uint8_t *data, uint32_t size) {

    memory_status_t status = MEM_OK;

    /* Check if a write is in progress */
    if (ns_firmware_write_addr == 0) status = MEM_UPDATE_NOT_STARTED_ERROR;
    if (status != MEM_OK) return status;

    /* Write the data */
    status = write_flash(ns_firmware_write_addr, data, size, false);
    if (status != MEM_OK) return status;

    /* Increment the write address */
    ns_firmware_write_addr += size;

    return status;
}


memory_status_t ns_firmware_update_finish() {

    memory_status_t status = MEM_OK;

    /* Check the hash of the firmware (also updates the valid flag) */
    if (!check_ns_firmware(OTHER_FLASH_BANK(hmeta.bank_swap))) status = MEM_HASH_INVALID_ERROR;
    if (status != MEM_OK) return status;

    /* Reset the write pointer */
    ns_firmware_write_addr = 0;

    return status;
}
