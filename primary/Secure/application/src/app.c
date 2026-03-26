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


static const bootloader_region_info_t app_regions[NUM_BANKS] = {
    {.addr = FLASH_NS_BANK1_BASE_ADDR + FLASH_NS_REGION_OFFSET,
     .size = FLASH_NS_REGION_SIZE},
    {.addr = FLASH_NS_BANK2_BASE_ADDR + FLASH_NS_REGION_OFFSET,
     .size = FLASH_NS_REGION_SIZE}
};

static const bootloader_region_info_t bootloader_regions[NUM_BANKS] = {
    {.addr = FLASH_S_BANK1_BASE_ADDR,
     .size = FLASH_S_REGION_SIZE + FLASH_NSC_REGION_SIZE},
    {.addr = FLASH_S_BANK2_BASE_ADDR,
     .size = FLASH_S_REGION_SIZE + FLASH_NSC_REGION_SIZE}
};

static const bootloader_config_t bootloader_config = {

    .app_count          = NUM_BANKS,
    .app_regions        = app_regions,
    .bootloader_count   = NUM_BANKS,
    .bootloader_regions = bootloader_regions,

    .log_buffer_base = (LOG_BASE),
    .log_buffer_size = (LOG_BUFFER_SIZE),

    .app_erase_ram_on_boot = true,
    .disable_tick_in_app   = false,
    .enable_uart_logging   = UART_LOGGING_ENABLE,
    .dual_banks            = true,
    .secure_bootloader     = true,
    .secure_app            = false,
};


int main(void) {

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
