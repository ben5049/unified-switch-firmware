/*
 * app.c
 *
 *  Created on: Mar 25, 2026
 *      Author: bens1
 */

#include "hal.h"
#include "usart.h"

#include "app.h"
#include "bootloader.h"
#include "bootloader_config.h"


static bootloader_region_info_t app_regions[NUM_BANKS];
static bootloader_region_info_t bootloader_regions[NUM_BANKS];
static bootloader_config_t      bootloader_config;


int main(void) {

    app_regions[0] = (bootloader_region_info_t) {
        .addr = FLASH_NS_REGION_START,
        .size = FLASH_NS_REGION_SIZE,
    };

    app_regions[1] = (bootloader_region_info_t) {
        .addr = FLASH2_NS_REGION_START,
        .size = FLASH2_NS_REGION_SIZE,
    };

    bootloader_regions[0] = (bootloader_region_info_t) {
        .addr = FLASH_S_REGION_START,
        .size = FLASH_S_REGION_SIZE + FLASH_NSC_REGION_SIZE,
    };

    bootloader_regions[1] = (bootloader_region_info_t) {
        .addr = FLASH2_S_REGION_START,
        .size = FLASH2_S_REGION_SIZE + FLASH_NSC2_REGION_SIZE,
    };

    bootloader_config = (bootloader_config_t) {

        .app_count          = NUM_BANKS,
        .app_regions        = app_regions,
        .bootloader_count   = NUM_BANKS,
        .bootloader_regions = bootloader_regions,

        .log_buffer_base = (LOG_BASE),
        .log_buffer_size = (LOG_BUFFER_SIZE),

        .app_erase_ram_on_boot = true,
        .disable_tick_in_app   = false,
        .enable_uart_logging   = false,
        .dual_banks            = true,
        .secure_bootloader     = true,
        .secure_app            = false,
    };

    /* Bootloader init does not return, it selects and jumps to the non-secure firmware */
    bootloader_init(&bootloader_config);
    while (1);
}


#if UART_LOGGING_ENABLE
int _write(int file, char *ptr, int len) {

    hal_status_t status = HAL_OK;

    while (huart4.gState != HAL_UART_STATE_READY);
    status = HAL_UART_Transmit(&huart4, (uint8_t *) ptr, len, 1000);

    if (status == HAL_OK) {
        return len;
    } else {
        return -1;
    }
}
#endif
