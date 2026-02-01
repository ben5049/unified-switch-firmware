/*
 * app_setup.c
 *
 *  Created on: 17 Aug 2025
 *      Author: bens1
 */

#include "app_setup.h"
#include "main.h"
#include "app_threadx.h"
#include "main.h"
#include "crc.h"
#include "dts.h"
#include "eth.h"
#include "icache.h"
#include "spi.h"
#include "gpio.h"
#include "aes.h"
#include "rtc.h"

#include "switch_thread.h"
#include "phy_thread.h"
#include "utils.h"
#include "static_checks.h"
#include "logging.h"


log_handle_t  hlog_setup, hlog_generic, hlog_phy, hlog_sw, hlog_comms, hlog_system, hlog_network;
log_handle_t *loggers[NUM_LOGGERS] = {&hlog_setup, &hlog_generic, &hlog_phy, &hlog_sw, &hlog_comms, &hlog_system, &hlog_network};


void app_setup(void) {

    /* Static assertions */
    CHECK_BASIC_TYPES();
    CHECK_DHCP_RECORD_MEMBERS();

    /* Start setup logger */
    log_status_t log_status;
    log_status = log_init(&hlog_setup, LOGGER_ID_ROOT_NS, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, &uwTick, LOGGING_TIMEOUT);
    if (log_status != LOGGING_OK) Error_Handler();
    LOG_INFO("Starting non-secure firmware");

    /* Start other loggers */
    log_status = log_init(&hlog_generic, LOGGER_ID_0, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, &uwTick, LOGGING_TIMEOUT);
    if (log_status != LOGGING_OK) Error_Handler();
    log_status = log_init(&hlog_phy, LOGGER_ID_1, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, &uwTick, LOGGING_TIMEOUT);
    if (log_status != LOGGING_OK) Error_Handler();
    log_status = log_init(&hlog_sw, LOGGER_ID_2, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, &uwTick, LOGGING_TIMEOUT);
    if (log_status != LOGGING_OK) Error_Handler();
    log_status = log_init(&hlog_comms, LOGGER_ID_3, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, &uwTick, LOGGING_TIMEOUT);
    if (log_status != LOGGING_OK) Error_Handler();
    log_status = log_init(&hlog_system, LOGGER_ID_4, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, &uwTick, LOGGING_TIMEOUT);
    if (log_status != LOGGING_OK) Error_Handler();
    log_status = log_init(&hlog_network, LOGGER_ID_5, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, &uwTick, LOGGING_TIMEOUT);
    if (log_status != LOGGING_OK) Error_Handler();

    /* Initialise important peripherals */
    MX_GPIO_Init();
    MX_ICACHE_Init();
    MX_RTC_Init();
    MX_CRC_Init();
    MX_SPI2_Init();
    LOG_INFO("Peripheral group 1 initialised");

    /* Change to FPWM mode for more accurate 3.3V rail (needed by PHYs) */
    set_3v3_regulator_to_FPWM();
    LOG_INFO("3V3 Regulator changed to FPWM mode");

    /* Initialise the switch */
    sja1105_status_t switch_status = switch_init(&hsw0);
    if (switch_status != SJA1105_OK) Error_Handler();
    LOG_INFO("SJA1105 Initialised");

    /* Ethernet MAC can now be initialised (requires switch REFCLK) */
    MX_ETH_Init();
    LOG_INFO("ETH Peripheral initialised");

    /* Initialise less important peripherals */
    MX_DTS_Init();
    MX_AES_Init();
    LOG_INFO("Peripheral group 2 initialised");
}
