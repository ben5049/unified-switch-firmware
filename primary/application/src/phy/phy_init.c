/*
 * phy_init.c
 *
 *  Created on: 11 Sept 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "main.h"

#include "phy_thread.h"
#include "phy_callbacks.h"
#include "config.h"
#include "utils.h"


phy_handle_dp83867_t hphy0;
phy_handle_88q211x_t hphy1, hphy2, hphy3, hphy4, hphy5;
phy_handle_lan867x_t hphy6;

static phy_config_dp83867_t phy_config_0;
static phy_config_88q211x_t phy_config_1, phy_config_2, phy_config_3, phy_config_4, phy_config_5;
static phy_config_lan867x_t phy_config_6;

void                         *phy_handles[NUM_PHYS]   = {&hphy0, &hphy1, &hphy2, &hphy3, &hphy4, &hphy5, &hphy6};
static void                  *phy_configs[NUM_PHYS]   = {&phy_config_0, &phy_config_1, &phy_config_2, &phy_config_3, &phy_config_4, &phy_config_5, &phy_config_6};
const static phy_callbacks_t *phy_callbacks[NUM_PHYS] = {&phy_callbacks_dp83867, &phy_callbacks_88q2112, &phy_callbacks_88q2112, &phy_callbacks_88q2112, &phy_callbacks_88q2112, &phy_callbacks_88q2112, &phy_callbacks_lan8671};


phy_status_t phys_init() {

    phy_status_t status = PHY_OK;

    /* Reset PHY handles */
    memset(&hphy0, 0, sizeof(phy_handle_dp83867_t));
    memset(&hphy1, 0, sizeof(phy_handle_88q211x_t));
    memset(&hphy2, 0, sizeof(phy_handle_88q211x_t));
    memset(&hphy3, 0, sizeof(phy_handle_88q211x_t));
    memset(&hphy4, 0, sizeof(phy_handle_88q211x_t));
    memset(&hphy5, 0, sizeof(phy_handle_88q211x_t));
    memset(&hphy6, 0, sizeof(phy_handle_lan867x_t));

    phy_config_0.variant       = PHY_VARIANT_DP83867;
    phy_config_0.phy_addr      = 0x03;
    phy_config_0.timeout       = PHY_TIMEOUT_MS;
    phy_config_0.interface     = PHY_INTERFACE_RGMII;
    phy_config_0.default_speed = PHY_SPEED_MBPS_TO_ENUM(PORT0_SPEED_MBPS);
    phy_config_0.default_role  = PHY_ROLE_MASTER;

    phy_config_1.variant               = PHY_VARIANT_88Q2112;
    phy_config_1.phy_addr              = 0x00;
    phy_config_1.timeout               = PHY_TIMEOUT_MS;
    phy_config_1.interface             = PHY_INTERFACE_RGMII;
    phy_config_1.default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT1_SPEED_MBPS);
    phy_config_1.default_role          = PHY_ROLE_MASTER;
    phy_config_1.tx_clk_internal_delay = true;
    phy_config_1.rx_clk_internal_delay = true;
    phy_config_1.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_2.variant               = PHY_VARIANT_88Q2112;
    phy_config_2.phy_addr              = 0x00;
    phy_config_2.timeout               = PHY_TIMEOUT_MS;
    phy_config_2.interface             = PHY_INTERFACE_RGMII;
    phy_config_2.default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT2_SPEED_MBPS);
    phy_config_2.default_role          = PHY_ROLE_MASTER;
    phy_config_2.tx_clk_internal_delay = true;
    phy_config_2.rx_clk_internal_delay = true;
    phy_config_2.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_3.variant               = PHY_VARIANT_88Q2112;
    phy_config_3.phy_addr              = 0x00;
    phy_config_3.timeout               = PHY_TIMEOUT_MS;
    phy_config_3.interface             = PHY_INTERFACE_RGMII;
    phy_config_3.default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT3_SPEED_MBPS);
    phy_config_3.default_role          = PHY_ROLE_MASTER;
    phy_config_3.tx_clk_internal_delay = true;
    phy_config_3.rx_clk_internal_delay = true;
    phy_config_3.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_4.variant               = PHY_VARIANT_88Q2112;
    phy_config_4.phy_addr              = 0x00;
    phy_config_4.timeout               = PHY_TIMEOUT_MS;
    phy_config_4.interface             = PHY_INTERFACE_RGMII;
    phy_config_4.default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT4_SPEED_MBPS);
    phy_config_4.default_role          = PHY_ROLE_MASTER;
    phy_config_4.tx_clk_internal_delay = true;
    phy_config_4.rx_clk_internal_delay = true;
    phy_config_4.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_5.variant               = PHY_VARIANT_88Q2112;
    phy_config_5.phy_addr              = 0x00;
    phy_config_5.timeout               = PHY_TIMEOUT_MS;
    phy_config_5.interface             = PHY_INTERFACE_RGMII;
    phy_config_5.default_speed         = PHY_SPEED_MBPS_TO_ENUM(PORT5_SPEED_MBPS);
    phy_config_5.default_role          = PHY_ROLE_MASTER;
    phy_config_5.tx_clk_internal_delay = true;
    phy_config_5.rx_clk_internal_delay = true;
    phy_config_5.fifo_size             = PHY_FIFO_SIZE_88Q211X_15KB;

    phy_config_6.variant         = PHY_VARIANT_LAN8671;
    phy_config_6.phy_addr        = 0x08;
    phy_config_6.timeout         = PHY_TIMEOUT_MS;
    phy_config_6.interface       = PHY_INTERFACE_RMII;
    phy_config_6.plca_enabled    = true;
    phy_config_6.plca_id         = PHY_PLCA_COORDINATOR_ID;
    phy_config_6.plca_node_count = PHY_PLCA_DEFAULT_NODE_COUNT; /* Maximum of 16 devices on the bus by default, all devices must have the same node count setting. */

    /* Set pins to a known state */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, SET);
    HAL_GPIO_WritePin(PHY_WAKE_GPIO_Port, PHY_WAKE_Pin, SET);
    HAL_GPIO_WritePin(PHY_CLK_EN_GPIO_Port, PHY_CLK_EN_Pin, SET);

    /* Select PHY 1 to start */
    HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_SET);

    /* Hardware reset all PHYs (TODO: Reduce times) */
    tx_thread_sleep_ms(10);
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, RESET);
    tx_thread_sleep_ms(10); /* 10ms required by 88Q2112 */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, SET);
    tx_thread_sleep_ms(10);

    /* Initialise all PHYs, setting the callback context to the port number */
    for (phy_index_t i = 0; i < NUM_PHYS; i++) {
        status = PHY_Init(phy_handles[i], phy_configs[i], phy_callbacks[i], (void *) i);
        if (status != PHY_OK) {
            ns_log_write("Failed to initialise PHY %d", i);
            Error_Handler();
        }
    }

    /* Enable interrupts TODO: Fix */
    status = PHY_88Q211X_EnableInterrupts(&hphy1);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableInterrupts(&hphy2);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableInterrupts(&hphy3);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableInterrupts(&hphy4);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableInterrupts(&hphy5);
    if (status != PHY_OK) Error_Handler();
    /* TODO: Enable other PHY interrupts */

    /* Enable the temperature sensors */
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy1);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy2);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy3);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy4);
    if (status != PHY_OK) Error_Handler();
    status = PHY_88Q211X_EnableTemperatureSensor(&hphy5);
    if (status != PHY_OK) Error_Handler();

    /* TODO: Perform other configuration */

    /* TODO: Enable End to End Transparent Clock and PTP hardware acceleration */

    return status;
}
