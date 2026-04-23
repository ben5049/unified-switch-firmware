/*
 * app.c
 *
 *  Created on: 17 Aug 2025
 *      Author: bens1
 */

#include "main.h"
#include "app_threadx.h"
#include "main.h"
#include "gtzc_ns.h"
#include "gpio.h"
#include "rtc.h"
#include "crc.h"
#include "spi.h"
#include "eth.h"
#include "adc.h"
#include "dts.h"
#include "aes.h"

#include "app.h"
#include "switch_thread.h"
#include "phy_thread.h"
#include "utils.h"
#include "static_checks.h"
#include "logging.h"


/* Private function prototypes */
static bool     test_nvm(void);
static uint32_t logging_timestamp_callback(void *context);


log_handle_t  hlog_setup, hlog_generic, hlog_phy, hlog_sw, hlog_comms, hlog_system, hlog_network;
log_handle_t *loggers[NUM_LOGGERS] = {&hlog_setup, &hlog_generic, &hlog_phy, &hlog_sw, &hlog_comms, &hlog_system, &hlog_network};


int main(void) {

    /* Static assertions */
    CHECK_BASIC_TYPES();

    /* MPU Configuration */
    mpu_config();

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* GTZC initialisation */
    MX_GTZC_NS_Init();

    /* Enable the DWT, needed for nanosecond delay functions */
    dwt_init();

    /* Test the non-volatile memory is working correctly */
    if (!test_nvm()) error_handler();

    /* Start setup logger */
    log_status_t log_status;
    log_status = log_init(&hlog_setup, LOGGER_ID_ROOT_NS, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
    if (log_status != LOGGING_OK) error_handler();
    LOG_INFO("Starting non-secure firmware");

    /* Start other loggers */
    log_status = log_init(&hlog_generic, LOGGER_ID_0, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
    if (log_status != LOGGING_OK) error_handler();
    log_status = log_init(&hlog_phy, LOGGER_ID_1, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
    if (log_status != LOGGING_OK) error_handler();
    log_status = log_init(&hlog_sw, LOGGER_ID_2, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
    if (log_status != LOGGING_OK) error_handler();
    log_status = log_init(&hlog_comms, LOGGER_ID_3, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
    if (log_status != LOGGING_OK) error_handler();
    log_status = log_init(&hlog_system, LOGGER_ID_4, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
    if (log_status != LOGGING_OK) error_handler();
    log_status = log_init(&hlog_network, LOGGER_ID_5, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
    if (log_status != LOGGING_OK) error_handler();

    /* Initialise important peripherals */
    MX_GPIO_Init();
    MX_RTC_Init();
    MX_CRC_Init();
    MX_SPI2_Init();
    LOG_INFO("Peripheral group 1 initialised");

    /* Change to FPWM mode for more accurate 3.3V rail (needed by PHYs) */
    set_3v3_regulator_to_FPWM();
    LOG_INFO("3V3 Regulator changed to FPWM mode");

    /* Initialise the switch */
    sja1105_status_t switch_status = switch_init();
    if (switch_status != SJA1105_OK) error_handler();
    LOG_INFO("SJA1105 Initialised");

    /* Ethernet MAC can now be initialised (requires switch REFCLK) */
    MX_ETH_Init();
    LOG_INFO("ETH Peripheral initialised");

    /* Initialise less important peripherals */
    MX_ADC2_Init();
    MX_DTS_Init();
    MX_AES_Init();
    LOG_INFO("Peripheral group 2 initialised");

    /* Shouldn't return */
    MX_ThreadX_Init();
    while (1);
}


/* Test the non-volatile memory is working. This is accessessed via the secure firmware */
static bool test_nvm() {

    bool     test_failed = false;
    uint32_t test_write;
    uint32_t test_read;
    int      bytes_written;
    int      bytes_read;

    static_assert(sizeof(test_write) == USER_STORAGE_TEST_SIZE);
    static_assert(sizeof(test_read) == USER_STORAGE_TEST_SIZE);

    /* Get a random number to test the NVM with */
    test_write = s_random_u32();

    /* Write the number */
    bytes_written = s_write_user_storage(USER_STORAGE_TEST_ADDR, (uint8_t *) &test_write, USER_STORAGE_TEST_SIZE);
    if (bytes_written != USER_STORAGE_TEST_SIZE) test_failed = true;
    if (test_failed) return false;

    /* Read the number back */
    bytes_read = s_read_user_storage(USER_STORAGE_TEST_ADDR, (uint8_t *) &test_read, USER_STORAGE_TEST_SIZE);
    if (bytes_read != USER_STORAGE_TEST_SIZE) test_failed = true;
    if (test_failed) return false;

    /* Check the write and read were successful */
    if (test_read != test_write) test_failed = true;
    return !test_failed;
}


static uint32_t logging_timestamp_callback(void *context) {

    /* Use kernel time only if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        return HAL_GetTick();
    } else {
        return tx_time_get_ms();
    }
}
