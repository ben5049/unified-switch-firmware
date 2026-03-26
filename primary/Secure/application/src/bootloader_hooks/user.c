/*
 * user.c
 *
 *  Created on: Mar 25, 2026
 *      Author: bens1
 */

#include "hal.h"
#include "main.h"
#include "gtzc_s.h"
#include "gpio.h"
#include "sau.h"
#include "icache.h"
#include "flash.h"
#include "ramcfg.h"
#include "rng.h"
#include "hash.h"
#include "pka.h"
#include "aes.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"

#include "bootloader.h"
#include "log_tools.h"

#include "app.h"
#include "backup.h"


bootloader_status_t bootloader_hardware_init() {

    /* MPU Configuration */
    mpu_config();

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    system_clock_config();

    /* Configure the peripherals common clocks */
    periph_common_clock_config();

    /* GTZC initialisation */
    MX_GTZC_S_Init();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SAU_Init();
    MX_ICACHE_Init();
    MX_FLASH_Init();
    MX_RAMCFG_Init();
    MX_RNG_Init();
    MX_HASH_Init();
    MX_PKA_Init();
    MX_SAES_AES_Init();
    MX_RTC_Init();
    MX_SPI1_Init();
    MX_UART4_Init();

    /* Enable the backup SRAM */
    enable_backup_domain();

    return BL_OK;
}


bootloader_status_t bootloader_user_init() {

    hal_status_t status = HAL_OK;

    /* Enable ECC interrupts */
    status = HAL_RAMCFG_EnableNotification(&hramcfg_SRAM2, RAMCFG_IT_ALL);
    if (status != HAL_OK) return BL_ERROR;
    status = HAL_RAMCFG_EnableNotification(&hramcfg_SRAM3, RAMCFG_IT_ALL);
    if (status != HAL_OK) return BL_ERROR;
    status = HAL_RAMCFG_EnableNotification(&hramcfg_BKPRAM, RAMCFG_IT_ALL);
    if (status != HAL_OK) return BL_ERROR;
    LOG_INFO("RAM ECC notifications enabled");

    return BL_OK;
}
