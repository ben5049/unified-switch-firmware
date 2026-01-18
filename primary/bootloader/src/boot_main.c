/*
 * boot_main.c
 *
 *  Created on: Aug 18, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdbool.h"
#include "gpio.h"
// #include "gpdma.h"
#include "flash.h"
// #include "sau.h"
#include "ramcfg.h"
#include "spi.h"
#include "rng.h"
#include "hash.h"
#include "pka.h"
#include "usart.h"
#include "aes.h"

#include "boot_main.h"
#include "boot_config.h"
#include "integrity.h"
#include "metadata.h"
#include "utils.h"
#include "memory_tools.h"
#include "logging.h"
#include "error.h"
#include "rtc.h"

uint8_t __attribute__((section(".LOG"))) log_buffer[LOG_BUFFER_SIZE];

__ALIGN_BEGIN static uint8_t current_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;
__ALIGN_BEGIN static uint8_t other_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;
__ALIGN_BEGIN static uint8_t current_non_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;
__ALIGN_BEGIN static uint8_t other_non_secure_firmware_hash[SHA256_SIZE] __ALIGN_END;

void boot_main() {

    static uint8_t status;
    bool           crash_recovery = hmeta.metadata.crashed;

    /* Step 1: Initialise peripherals
     * Step 2: Get previous secure firmware version and CRC from FRAM for current bank
     *   Step 2a: If current version is newer then compute CRC of the secure region (FLASH_S and FLASH_NSC) and store
     *   Step 2b: If current version is the same or older or then compute CRC of FLASH_S and FLASH_NSC and compare. If corrupted trigger a bank swap (back to step 1)
     * Step 3: Get previous secure firmware version and CRC from FRAM for other bank
     *   Step 3a: If current version is newer then copy current bank secure region into other bank
     *   Step 3b: If current version is the same compute the CRC of the other bank. If corrupted then copy current bank secure region into other bank
     *   Step 3c: If current version is older then something is wrong. Trigger bank swap and it will force the other bank to repeat these steps and update current bank
     * Step 4: Compute the CRC of both non-secure regions and compare with stored value.
     *   Step 4a: If other isn't corrupted and current isn't then continue
     *   Step 4b: If other is corrupted and current isn't then copy current into other
     *   Step 4c: If other isn't corrupted and current is then copy other into current
     *   Step 4d: If both are corrupted then panic
     */

    /* Initialise peripherals */
    // MX_GPIO_Init();
    // MX_GPDMA1_Init();
    MX_FLASH_Init();
    MX_RAMCFG_Init();
    MX_SPI1_Init();
    MX_RNG_Init();
    MX_HASH_Init();
    MX_PKA_Init();
    MX_UART4_Init();
    MX_SAES_AES_Init();
    MX_RTC_Init();

    /* Enable writing to the backup SRAM */
    enable_backup_domain();

    /* Enable logging */
    status = log_init(&hlog, log_buffer, LOG_BUFFER_SIZE);
    CHECK_STATUS_LOG(status);
    LOG_INFO("Starting secure firmware\n");

#ifdef DEBUG // TODO: remove?
    crash_recovery    = false;
    hmeta.initialised = false;
#endif

    /* Enable ECC interrupts */
    status = HAL_RAMCFG_EnableNotification(&hramcfg_SRAM2, RAMCFG_IT_ALL);
    CHECK_STATUS(status, HAL_OK, ERROR_HAL);
    status = HAL_RAMCFG_EnableNotification(&hramcfg_SRAM3, RAMCFG_IT_ALL);
    CHECK_STATUS(status, HAL_OK, ERROR_HAL);
    status = HAL_RAMCFG_EnableNotification(&hramcfg_BKPRAM, RAMCFG_IT_ALL);
    CHECK_STATUS(status, HAL_OK, ERROR_HAL);
    LOG_INFO("RAM ECC notifications enabled\n");

    /* Get bank swap from option bytes */
    bool bank_swap = get_bank_swap();
    LOG_INFO("Executing from bank %u\n", (uint8_t) bank_swap + 1);

    /* Check if the metadata module is already initialised (CPU reset, but memory persisted) */
    if (hmeta.initialised == true && !crash_recovery) {
        status = META_Reinit(&hmeta, bank_swap);
        CHECK_STATUS_META(status);
    }

    /* Initialise the metadata module (uses FRAM over SPI1) */
    else {
        status = META_Init(&hmeta, bank_swap);
        CHECK_STATUS_META(status);
    }

#ifdef DEBUG // TODO: remove?
    hmeta.metadata.crashed = false;
    hmeta.first_boot       = true;
#endif

    /* Check if the last boot was a crash. TODO: Startup non-secure firmware in crash recovery mode */
    crash_recovery |= hmeta.metadata.crashed;
    if (crash_recovery)
        LOG_INFO("Last shutdown was due to a crash\n");

    /* Initialise the integrity module and pass in empty buffers for hash digests */
    status = INTEGRITY_Init(bank_swap, current_secure_firmware_hash, other_secure_firmware_hash, current_non_secure_firmware_hash, other_non_secure_firmware_hash);
    CHECK_STATUS_INTEGRITY(status);

    /* Calculate the SHA256 of the current secure firmware */
    status = INTEGRITY_compute_s_firmware_hash(CURRENT_FLASH_BANK(bank_swap));
    CHECK_STATUS_INTEGRITY(status);
    LOG_INFO_SHA256("Current secure firmware hash = %s\n", current_secure_firmware_hash);

    /* If this is the first boot then configure the device and metadata */
    if (hmeta.first_boot) {

        /* Configure metadata */
        status = META_Configure(&hmeta);
        CHECK_STATUS_META(status);

        /* Store the secure firmware hash */
        status = META_set_s_firmware_hash(&hmeta, CURRENT_FLASH_BANK(bank_swap), current_secure_firmware_hash);
        CHECK_STATUS_INTEGRITY(status);

        /* Get the non-secure firmware hash and store it */
        status = INTEGRITY_compute_ns_firmware_hash(CURRENT_FLASH_BANK(bank_swap));
        CHECK_STATUS_INTEGRITY(status);
        status = META_set_ns_firmware_hash(&hmeta, CURRENT_FLASH_BANK(bank_swap), current_non_secure_firmware_hash);
        CHECK_STATUS_INTEGRITY(status);

        /* Copy current secure and non-secure firmwares into the other bank and check they were written correctly */
        status = copy_s_firmware_to_other_bank(bank_swap);
        CHECK_STATUS_MEM(status);
        status = copy_ns_firmware(FLASH_BANK_1, FLASH_BANK_2);
        CHECK_STATUS_MEM(status);
    }

    /* Else this isn't the first boot */
    else {

        /* Check the current secure firmware hasn't been corrupted or tampered with */
        bool valid = true;
        status     = META_check_s_firmware_hash(&hmeta, CURRENT_FLASH_BANK(bank_swap), current_secure_firmware_hash, &valid);
        CHECK_STATUS_META(status);

        /* If the secure firmware has been corrupted or tampered with then swap banks if the other bank is valid.
         * Normally this shouldn't return as it needs a system reset for swapping banks take effect */
        bool other_valid = bank_swap ? hmeta.metadata.s_firmware_1_valid : hmeta.metadata.s_firmware_2_valid;
        if (!valid && other_valid) {
            swap_banks();
            error_handler(ERROR_GENERIC, 0);
        } else {
            error_handler(ERROR_GENERIC, 0);
        }

        /* Check the other firmwares haven't been corrupted or tampered with */

        /* Calculate hash of the other secure firmware */
        valid = check_s_firmware(OTHER_FLASH_BANK(bank_swap));

        /* If corrupted then repair */
        if (!valid) {
            LOG_INFO("Other secure firmware image invalid. Overwriting\n");
            status = copy_s_firmware_to_other_bank(bank_swap);
            CHECK_STATUS_MEM(status);
        }

        /* Calculate hash of the non-secure firmwares */
        bool ns_firmware_1_valid = check_ns_firmware(FLASH_BANK_1);
        bool ns_firmware_2_valid = check_ns_firmware(FLASH_BANK_2);

        /* Both firmware images valid */
        if (ns_firmware_1_valid && ns_firmware_2_valid) {
            LOG_INFO("Both non-secure firmware images valid\n");
        }

        /* Bank 1 firmware invalid, bank 2 valid: repair */
        else if (!ns_firmware_1_valid && ns_firmware_2_valid) {
            LOG_INFO("Non-secure firmware image 1 isn't valid. Overwriting with image 2\n");
            status = copy_ns_firmware(FLASH_BANK_2, FLASH_BANK_1);
            CHECK_STATUS_MEM(status);
        }

        /* Bank 2 firmware invalid, bank 1 valid: repair */
        else if (ns_firmware_1_valid && !ns_firmware_2_valid) {
            LOG_INFO("Non-secure firmware image 2 isn't valid. Overwriting with image 1\n");
            status = copy_ns_firmware(FLASH_BANK_1, FLASH_BANK_2);
            CHECK_STATUS_MEM(status);
        }

        /* Both firmwares invalid. We are cooked */
        else {
            LOG_INFO("No valid non-secure firmware images\n");
            error_handler(ERROR_GENERIC, 0);
        }
    }

    /* Erase SRAM3 to prevent ECC errors due to uninitialised memory */
    LOG_INFO("Erasing SRAM3\n");
    status = HAL_RAMCFG_Erase(&hramcfg_SRAM3);
    CHECK_STATUS(status, HAL_OK, ERROR_HAL);

    /* Boot is over */
    hmeta.first_boot = false;

    LOG_INFO("Jumping to non-secure firmware\n");

#if DISABLE_S_SYSTICK_IN_NS == true
    /* Secure SysTick should be suspended before calling non-secure init in
     * order to avoid waking up from a sleep mode entered by non-secure firmware.
     * The Secure SysTick shall be resumed on non-secure callable functions */
    HAL_SuspendTick();
#endif
}
