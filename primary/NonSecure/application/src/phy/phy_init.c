/*
 * phy_init.c
 *
 *  Created on: 11 Sept 2025
 *      Author: bens1
 */

#include "stdbool.h"
#include "stdint.h"
#include "main.h"

#include "phy_thread.h"
#include "phy_callbacks.h"
#include "config.h"
#include "utils.h"


#if HW_VERSION == 4

phy_handle_88q211x_t              hphy0;
static const phy_config_88q211x_t phy_config_0 = {
    .variant               = PHY_VARIANT_88Q2112,
    .phy_addr              = 0x00,
    .c45_en                = true,
    .timeout               = PHY_TIMEOUT_MS,
    .interface             = PHY_INTERFACE_RGMII,
    .default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT0_SPEED_MBPS),
    .default_role          = PHY_ROLE_SLAVE,
    .tx_clk_internal_delay = true,
    .rx_clk_internal_delay = true,
    .fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB,
};

#elif HW_VERSION == 5

phy_handle_dp83867_t              hphy0;
static const phy_config_dp83867_t phy_config_0 = {
    .variant       = PHY_VARIANT_DP83867,
    .phy_addr      = 0x03,
    .c45_en        = false,
    .timeout       = PHY_TIMEOUT_MS,
    .interface     = PHY_INTERFACE_RGMII,
    .default_speed = PHY_SPEED_MBPS_TO_ENUM(PORT0_SPEED_MBPS),
    .default_role  = PHY_ROLE_MASTER,
};

#endif

phy_handle_88q211x_t              hphy1;
static const phy_config_88q211x_t phy_config_1 = {
    .variant = PHY_VARIANT_88Q2112,
#if HW_VERSION == 4
    .phy_addr = 0x01,
#elif HW_VERSION == 5
    .phy_addr = 0x00,
#endif
    .c45_en                = true,
    .timeout               = PHY_TIMEOUT_MS,
    .interface             = PHY_INTERFACE_RGMII,
    .default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT1_SPEED_MBPS),
    .default_role          = PHY_ROLE_MASTER,
    .tx_clk_internal_delay = true,
    .rx_clk_internal_delay = true,
    .fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB,
};

phy_handle_88q211x_t              hphy2;
static const phy_config_88q211x_t phy_config_2 = {
    .variant = PHY_VARIANT_88Q2112,
#if HW_VERSION == 4
    .phy_addr = 0x02,
#elif HW_VERSION == 5
    .phy_addr = 0x00,
#endif
    .c45_en                = true,
    .timeout               = PHY_TIMEOUT_MS,
    .interface             = PHY_INTERFACE_RGMII,
    .default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT2_SPEED_MBPS),
    .default_role          = PHY_ROLE_MASTER,
    .tx_clk_internal_delay = true,
    .rx_clk_internal_delay = true,
    .fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB,
};

#if HW_VERSION == 4

phy_handle_lan867x_t              hphy3;
static const phy_config_lan867x_t phy_config_3 = {
    .variant         = PHY_VARIANT_LAN8671,
    .phy_addr        = 0x08,
    .c45_en          = false,
    .timeout         = PHY_TIMEOUT_MS,
    .interface       = PHY_INTERFACE_RMII,
    .plca_enabled    = true,
    .plca_id         = PHY_PLCA_COORDINATOR_ID,
    .plca_node_count = PHY_PLCA_DEFAULT_NODE_COUNT, /* Maximum of 16 devices on the bus by default, all devices must have the same node count setting. */
};

#elif HW_VERSION == 5

phy_handle_88q211x_t              hphy3;
static const phy_config_88q211x_t phy_config_3 = {
    .variant               = PHY_VARIANT_88Q2112,
    .phy_addr              = 0x00,
    .c45_en                = true,
    .timeout               = PHY_TIMEOUT_MS,
    .interface             = PHY_INTERFACE_RGMII,
    .default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT3_SPEED_MBPS),
    .default_role          = PHY_ROLE_MASTER,
    .tx_clk_internal_delay = true,
    .rx_clk_internal_delay = true,
    .fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB,
};

phy_handle_88q211x_t              hphy4;
static const phy_config_88q211x_t phy_config_4 = {
    .variant               = PHY_VARIANT_88Q2112,
    .phy_addr              = 0x00,
    .c45_en                = true,
    .timeout               = PHY_TIMEOUT_MS,
    .interface             = PHY_INTERFACE_RGMII,
    .default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT4_SPEED_MBPS),
    .default_role          = PHY_ROLE_MASTER,
    .tx_clk_internal_delay = true,
    .rx_clk_internal_delay = true,
    .fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB,
};

phy_handle_88q211x_t              hphy5;
static const phy_config_88q211x_t phy_config_5 = {
    .variant               = PHY_VARIANT_88Q2112,
    .phy_addr              = 0x00,
    .c45_en                = true,
    .timeout               = PHY_TIMEOUT_MS,
    .interface             = PHY_INTERFACE_RGMII,
    .default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT5_SPEED_MBPS),
    .default_role          = PHY_ROLE_MASTER,
    .tx_clk_internal_delay = true,
    .rx_clk_internal_delay = true,
    .fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB,
};

phy_handle_lan867x_t              hphy6;
static const phy_config_lan867x_t phy_config_6 = {
    .variant         = PHY_VARIANT_LAN8671,
    .phy_addr        = 0x08,
    .c45_en          = false,
    .timeout         = PHY_TIMEOUT_MS,
    .interface       = PHY_INTERFACE_RMII,
    .plca_enabled    = true,
    .plca_id         = PHY_PLCA_COORDINATOR_ID,
    .plca_node_count = PHY_PLCA_DEFAULT_NODE_COUNT, /* Maximum of 16 devices on the bus by default, all devices must have the same node count setting. */
};

#endif

void *phy_handles[NUM_PHYS] = {
    &hphy0,
    &hphy1,
    &hphy2,
    &hphy3,
#if HW_VERSION == 5
    &hphy4,
    &hphy5,
    &hphy6
#endif
};

static const void *phy_configs[NUM_PHYS] = {
    &phy_config_0,
    &phy_config_1,
    &phy_config_2,
    &phy_config_3,
#if HW_VERSION == 5
    &phy_config_4,
    &phy_config_5,
    &phy_config_6
#endif
};

const static phy_callbacks_t *phy_callbacks[NUM_PHYS] = {
#if HW_VERSION == 4
    &phy_callbacks_88q2112,
#elif HW_VERSION == 5
    &phy_callbacks_dp83867,
#endif
    &phy_callbacks_88q2112,
    &phy_callbacks_88q2112,
#if HW_VERSION == 4
    &phy_callbacks_lan8671,
#elif HW_VERSION == 5
    &phy_callbacks_88q2112,
    &phy_callbacks_88q2112,
    &phy_callbacks_88q2112,
    &phy_callbacks_lan8671
#endif
};


phy_status_t phys_init() {

    phy_status_t status = PHY_OK;

    /* Reset shared structs */
#if NUM_PHYS > 0
    memset(&hphy0, 0, sizeof(hphy0));
#endif
#if NUM_PHYS > 1
    memset(&hphy1, 0, sizeof(hphy1));
#endif
#if NUM_PHYS > 2
    memset(&hphy2, 0, sizeof(hphy2));
#endif
#if NUM_PHYS > 3
    memset(&hphy3, 0, sizeof(hphy3));
#endif
#if NUM_PHYS > 4
    memset(&hphy4, 0, sizeof(hphy4));
#endif
#if NUM_PHYS > 5
    memset(&hphy5, 0, sizeof(hphy5));
#endif
#if NUM_PHYS > 6
    memset(&hphy6, 0, sizeof(hphy6));
#endif

    /* Set pins to a known state */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PHY_WAKE_GPIO_Port, PHY_WAKE_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PHY_CLK_EN_GPIO_Port, PHY_CLK_EN_Pin, GPIO_PIN_SET);

    /* Select PHY 1 to start */
#if HW_VERSION == 5
    HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_SET);
#endif

    /* Hardware reset all PHYs (TODO: Reduce times) */
    tx_thread_sleep_ms(2);
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, GPIO_PIN_RESET);
    tx_thread_sleep_ms(10); /* 10ms required by 88Q2112 */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, GPIO_PIN_SET);
    tx_thread_sleep_ms(2);

    /* Initialise all PHYs, setting the callback context to the port number */
    for (phy_index_t i = 0; i < NUM_PHYS; i++) {
        status = PHY_Init(phy_handles[i], phy_configs[i], phy_callbacks[i], (void *) i);
        if (status != PHY_OK) {
            LOG_ERROR("Failed to initialise PHY %d", i);
            Error_Handler();
        }
    }

    /* Clear the interrupt flags since the LAN8671 init contains a software reset
     * that triggers a non-maskable interrupt. While it is internally cleared
     * in the driver, the GPIO interrupt handler will have fired, and when we
     * try to process the interrupt, the source won't be found. */
    if (tx_event_flags_set(&phy_events_handle, 0, TX_AND) != TX_SUCCESS) status = PHY_ERROR;
    if (status != PHY_OK) return status;

    /* Less critical setup: interrupts & temperature sensors */
    for (phy_index_t i = 0; i < NUM_PHYS; i++) {
        status = PHY_EnableInterrupts(phy_handles[i]);
        if (status != PHY_OK) Error_Handler();
        status = PHY_EnableTemperatureSensor(phy_handles[i]);
        if (status != PHY_OK) Error_Handler();
    }

    /* TODO: Perform other configuration */

    /* TODO: Enable End to End Transparent Clock and PTP hardware acceleration */

    return status;
}
