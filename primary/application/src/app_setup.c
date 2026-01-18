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
// #include "gpdma.h"
#include "rng.h"
#include "aes.h"
#include "rtc.h"

#include "switch_thread.h"
#include "phy_thread.h"
#include "utils.h"


void app_setup(void) {

    ns_log_write("Starting non-secure firmware\n");

    /* Initialise important peripherals */
    MX_GPIO_Init();
    MX_ICACHE_Init();
    MX_RTC_Init();
    MX_CRC_Init();
    MX_SPI2_Init();
    ns_log_write("Peripheral group 1 initialised\n");

    /* Change to FPWM mode for more accurate 3.3V rail (needed by PHYs) */
    set_3v3_regulator_to_FPWM();
    ns_log_write("3V3 Regulator changed to FPWM mode\n");

    /* Initialise the switch */
    sja1105_status_t switch_status = switch_init(&hsw0);
    if (switch_status != SJA1105_OK) Error_Handler();
    ns_log_write("SJA1105 Initialised\n");

    /* Ethernet MAC can now be initialised (requires switch REFCLK) */
    MX_ETH_Init();
    ns_log_write("ETH Peripheral initialised\n");

    /* Initialise less important peripherals */
    // MX_GPDMA1_Init();
    MX_DTS_Init();
    MX_RNG_Init();
    MX_AES_Init();
    ns_log_write("Peripheral group 2 initialised\n");
}
