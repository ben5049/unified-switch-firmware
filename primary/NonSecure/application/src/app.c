/*
 * app.c
 *
 *  Created on: 17 Aug 2025
 *      Author: bens1
 */

#include "assert.h"

#include "main.h"
#include "app_threadx.h"
#include "hal.h"
#include "gtzc_ns.h"
#include "gpio.h"
#include "rtc.h"
#include "crc.h"
#include "spi.h"
#include "eth.h"
#include "adc.h"
#include "dts.h"
#include "aes.h"
#include "secure_nsc.h"

#include "app.h"
#include "switch_thread.h"
#include "switch_utils.h"
#include "phy_thread.h"
#include "utils.h"
#include "static_checks.h"
#include "validation.h"


/* Private function prototypes */
static bool     test_nvm(void);
static uint32_t logging_timestamp_callback(void *context);


log_handle_t  hlog_setup, hlog_generic, hlog_phy, hlog_sw, hlog_comms, hlog_system, hlog_network, hlog_ptp;
log_handle_t *loggers[NUM_LOGGERS] = {&hlog_setup, &hlog_generic, &hlog_phy, &hlog_sw, &hlog_comms, &hlog_system, &hlog_network, &hlog_ptp};


int main(void) {

    log_status_t     log_status    = LOGGING_OK;
    sja1105_status_t switch_status = SJA1105_OK;

    /* Static assertions */
    CHECK_BASIC_TYPES();

    /* Enable peripheral clocks */
    periph_common_clock_config();

    /* MPU Configuration */
    mpu_config();

    /* Reset all peripherals, initialises the flash interface and systick. */
    HAL_Init();

    /* Initialise very important peripherals */
    MX_GTZC_NS_Init();
    systick_enable_pre_rtos();

    /* Test the non-volatile memory is working correctly */
    if (!test_nvm()) error_handler();

    /* Start setup logger */
    log_status = log_init(&hlog_setup, LOGGER_ID_ROOT_NS, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
    LOG_CHECK(log_status);
    LOG_INFO("Starting non-secure firmware");

    /* Start other loggers */
    for (uint_fast8_t i = 1; i < NUM_LOGGERS; i++) {
        log_status = log_init(loggers[i], LOGGER_ID_0 + i, (uint8_t *) LOG_BASE, LOG_BUFFER_SIZE, LOGGING_TIMEOUT, &logging_timestamp_callback, NULL);
        LOG_CHECK(log_status);
    }

#if UART_LOGGING_ENABLE
    if (!s_uart_logging_enabled()) LOG_WARNING("UART Logging not enabled in secure firmware");
#endif

    /* Initialise validation */
    VAL_INIT();

    /* Print boot info */
    LOG_INFO("Cold boot = %d", s_cold_boot());

    /* Initialise important peripherals */
    MX_GPIO_Init();
    MX_RTC_Init();
    MX_CRC_Init();
    MX_SPI2_Init();
    LOG_INFO("Peripheral group 1 initialised");

    /* Change to FPWM mode for more accurate 3.3V rail (needed by PHYs) */
    set_3v3_regulator_to_fpwm();
    LOG_INFO("3V3 Regulator changed to FPWM mode");

    /* Initialise the switch */
    switch_status = switch_init();
    SWITCH_CHECK(switch_status);
    LOG_INFO("SJA1105(s) initialised");

    /* Ethernet MAC can now be initialised (requires switch REFCLK) */
    SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1QEN); /* Enable clk_ptp_ref_i */
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


int _write(int file, char *ptr, int len) {
    return s_write(file, ptr, len);
}


void __assert_func(const char *file, int line, const char *func, const char *failedexpr) {
    LOG_ERROR_NO_CHECK("Assert failed at '%s' line %d", file, line); /* Don't check return since assertions can fail before logging is enabled */
    error_handler();
    while (1);
}
