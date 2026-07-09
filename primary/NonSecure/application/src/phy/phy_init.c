/*
 * phy_init.c
 *
 *  Created on: 11 Sept 2025
 *      Author: bens1
 */

#include "stdbool.h"
#include "stdint.h"

#include "app.h"
#include "phy.h"
#include "utils.h"
#include "validation.h"


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
    .default_role  = PHY_ROLE_SLAVE,
    .clk125        = false,
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
    .c45_en        = true,
    .timeout       = PHY_TIMEOUT_MS,
    .interface     = PHY_INTERFACE_RGMII,
    .default_speed = PHY_SPEED_MBPS_TO_ENUM(PORT1_SPEED_MBPS),
#if HW_VERSION == 4
    .default_role = PHY_ROLE_MASTER,
#elif HW_VERSION == 5
    .default_role = PHY_ROLE_SLAVE,
#endif
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

phy_handle_base_t *phy_handles[NUM_PHYS] = {
    (phy_handle_base_t *) &hphy0,
    (phy_handle_base_t *) &hphy1,
    (phy_handle_base_t *) &hphy2,
    (phy_handle_base_t *) &hphy3,
#if HW_VERSION == 5
    (phy_handle_base_t *) &hphy4,
    (phy_handle_base_t *) &hphy5,
    (phy_handle_base_t *) &hphy6
#endif
};

static const phy_config_base_t *phy_configs[NUM_PHYS] = {
    (phy_config_base_t *) &phy_config_0,
    (phy_config_base_t *) &phy_config_1,
    (phy_config_base_t *) &phy_config_2,
    (phy_config_base_t *) &phy_config_3,
#if HW_VERSION == 5
    (phy_config_base_t *) &phy_config_4,
    (phy_config_base_t *) &phy_config_5,
    (phy_config_base_t *) &phy_config_6
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

static phy_info_t phy_info[NUM_PHYS];

uint16_t phy_ingress_latencies[NUM_PHYS] = {0};
uint16_t phy_egress_latencies[NUM_PHYS]  = {0};


phy_status_t phys_init() {

    phy_status_t status       = PHY_OK;
    uint32_t     current_time = tx_time_get_ms();

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
#if NUM_PHYS > 7
#error "Unsupported number of PHYs"
#endif

    /* Reset PHY info */
    for (phy_index_t i = 0; i < NUM_PHYS; i++) {
        phy_info[i].index                  = i;
        phy_info[i].connection_state       = PHY_STATE_UNINITIALISED;
        phy_info[i].next_update_time       = current_time;
        phy_info[i].wait_for_link_interval = PHY_WAITING_FOR_LINK_INTERVAL;
        phy_info[i].link_attempts          = ((PHY_POLL_STAGGER_TIME * i) / PHY_RECONNECT_INTERVAL) % PHY_RECONNECT_ATTEMPTS;
        phy_info[i].last_self_test_time    = 0;
        phy_info[i].sqi                    = 0;
    }

    /* Set pins to a known state */
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PHY_WAKE_GPIO_Port, PHY_WAKE_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PHY_CLK_EN_GPIO_Port, PHY_CLK_EN_Pin, GPIO_PIN_SET);

    /* Select PHY 1 to start */
#if HW_VERSION == 5
    status = select_phy(PHY1_88Q2112, NULL);
    if (status != PHY_OK) return status;
#endif

    /* Hardware reset all PHYs (TODO: Reduce times) */
    tx_thread_sleep_ms(1);
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, GPIO_PIN_RESET);
    delay_ns(PHY_T_RESET_WIDTH);
    HAL_GPIO_WritePin(PHY_RST_GPIO_Port, PHY_RST_Pin, GPIO_PIN_SET);
    delay_ns(PHY_T_RESET_MDIO);

    /* Initialise all PHYs */
    for (phy_index_t i = 0; i < NUM_PHYS; i++) {
        status = PHY_Init(phy_handles[i], phy_configs[i], phy_callbacks[i], &phy_info[i]);
        if (status != PHY_OK) {
            LOG_ERROR("Failed to initialise PHY %d", i);
            VAL_TERMINATE();
            phy_info[i].connection_state = PHY_STATE_ERROR_UNRECOVERABLE; /* Allowed to continue in degraded state */
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
        if ((status != PHY_OK) && (status != PHY_NOT_IMPLEMENTED_ERROR)) error_handler(); // TODO: re-enable when implemented
        status = PHY_EnableTemperatureSensor(phy_handles[i]);
        PHY_CHECK(status);
    }

    /* Update PHY info with info from initialisation */
    for (phy_index_t i = 0; i < NUM_PHYS; i++) {
        phy_info[i].wait_for_link_interval = phy_handles[i]->autoneg ? PHY_WAITING_FOR_LINK_INTERVAL_AUTONEG : PHY_WAITING_FOR_LINK_INTERVAL;
    }

    return status;
}
