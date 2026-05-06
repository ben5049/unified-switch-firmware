/*
 * switch_init.c
 *
 *  Created on: Sep 04, 2025
 *      Author: bens1
 */

#include "stdalign.h"
#include "stdatomic.h"

#include "main.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "switch_utils.h"
#include "sja1105.h"
#include "utils.h"

#if HW_VERSION == 4
#include "swv4_sja1105_static_config_default.h"
#elif HW_VERSION == 5
#include "swv5_0_sja1105_static_config_default.h"
#include "swv5_1_sja1105_static_config_default.h"
#endif


sja1105_handle_t        switch_handles[NUM_SWITCHES];
static sja1105_config_t switch_configs[NUM_SWITCHES];
switch_info_t           switch_info[NUM_SWITCHES];
alignas(32) static uint32_t switch_fixed_length_table_buffers[NUM_SWITCHES][SJA1105_FIXED_BUFFER_SIZE];

static const uint32_t *switch_static_configs[NUM_SWITCHES] = {
#if HW_VERSION == 4
    swv4_sja1105_static_config_default
#elif HW_VERSION == 5
    swv5_0_sja1105_static_config_default,
    swv5_1_sja1105_static_config_default
#endif
};

static const uint32_t switch_static_config_sizes[NUM_SWITCHES] = {
#if HW_VERSION == 4
    SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE
#elif HW_VERSION == 5
    SWV5_0_SJA1105_STATIC_CONFIG_DEFAULT_SIZE,
    SWV5_1_SJA1105_STATIC_CONFIG_DEFAULT_SIZE
#endif
};

static const uint16_t port_speeds[NUM_PHYS] = {
    PORT0_SPEED_MBPS,
    PORT1_SPEED_MBPS,
    PORT2_SPEED_MBPS,
    PORT3_SPEED_MBPS,
#if HW_VERSION == 5
    PORT4_SPEED_MBPS,
    PORT5_SPEED_MBPS,
    PORT6_SPEED_MBPS,
#endif
};


sja1105_status_t switch_init() {

    sja1105_status_t status = SJA1105_OK;
    sja1105_port_t   port_config;

    /* Reset switch handles */
    memset(&switch_handles, 0, sizeof(switch_handles));

    /* Reset info structs */
    for (switch_index_t i = 0; i < NUM_SWITCHES; i++) {
        switch_info[i].index             = i;
        switch_info[i].temperature       = 0.0;
        switch_info[i].temperature_valid = false;
    }

    /* Reset the switches */
    HAL_GPIO_WritePin(SWCH_RST_GPIO_Port, SWCH_RST_Pin, GPIO_PIN_RESET);
    delay_ns(SJA1105_T_RST);            /* 5us delay */
    HAL_GPIO_WritePin(SWCH_RST_GPIO_Port, SWCH_RST_Pin, GPIO_PIN_SET);
    delay_ns(SJA1105_T_RST_STARTUP_HW); /* 329us minimum until SPI commands can be written (SJA1105_T_RST_STARTUP_HW). Must be blocking since TX kernel hasn't started */

    /* Check SPI parameters */
#if DEBUG
    if (SWCH_SPI.Init.DataSize != SPI_DATASIZE_32BIT) status = SJA1105_PARAMETER_ERROR;
    if (SWCH_SPI.Init.CLKPolarity != SPI_POLARITY_LOW) status = SJA1105_PARAMETER_ERROR;
    if (SWCH_SPI.Init.CLKPhase != SPI_PHASE_2EDGE) status = SJA1105_PARAMETER_ERROR;
    if (SWCH_SPI.Init.NSS != SPI_NSS_SOFT) status = SJA1105_PARAMETER_ERROR;
    if (SWCH_SPI.Init.FirstBit != SPI_FIRSTBIT_MSB) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;
#endif

    /* Initialise the ThreadX byte pools */
    status = switch_byte_pool_init_all();
    if (status != SJA1105_OK) return status;

    /* Set the general switch 0 parameters */
    switch_configs[0].variant      = VARIANT_SJA1105Q;
    switch_configs[0].timeout      = SWITCH_TIMEOUT_MS;
    switch_configs[0].mgmt_timeout = SWITCH_MANAGMENT_ROUTE_TIMEOUT_MS;
    switch_configs[0].host_port    = SW0_PORT_HOST;  /* STM32 Host port */
#if HW_VERSION == 4
    switch_configs[0].casc_port = SJA1105_NUM_PORTS; /* No cascaded port */
#elif HW_VERSION == 5
    switch_configs[0].casc_port = SW0_PORT_SW1; /* Port to switch 1 downstream */
#endif
    switch_configs[0].skew_clocks = true; /* Improves EMI performance */
    switch_configs[0].switch_id   = 0;

    /* Set the general switch 1 parameters */
#if HW_VERSION == 5
    switch_configs[1].variant      = VARIANT_SJA1105Q;
    switch_configs[1].timeout      = SWITCH_TIMEOUT_MS;
    switch_configs[1].mgmt_timeout = SWITCH_MANAGMENT_ROUTE_TIMEOUT_MS;
    switch_configs[1].host_port    = SW1_PORT_SW0;      /* Port to switch 0 upstream */
    switch_configs[1].casc_port    = SJA1105_NUM_PORTS; /* No downstream switches */
    switch_configs[1].skew_clocks  = true;              /* Improves EMI performance */
    switch_configs[1].switch_id    = 1;
#endif

    /* Switch 0 port 0 config */
#if HW_VERSION == 4
    port_config.port_num      = SW0_PORT_PHY0_88Q2112;
    port_config.interface     = SJA1105_INTERFACE_RGMII;
    port_config.mode          = SJA1105_MODE_MAC;
    port_config.speed         = SJA1105_SPEED_DYNAMIC;
    port_config.voltage       = SJA1105_IO_1V8;
    port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
#elif HW_VERSION == 5
    port_config.port_num                  = SW0_PORT_SW1;
    port_config.interface                 = SJA1105_INTERFACE_RGMII;
    port_config.mode                      = SJA1105_MODE_MAC;
    port_config.speed                     = SJA1105_SPEED_1G;
    port_config.voltage                   = SJA1105_IO_1V8;
    port_config.rgmii_id_mode             = SJA1105_RGMII_ID_TX_RX_1NS; /* Delays handled by switch 0 */
    port_config.connected_switch_port_num = SW1_PORT_SW0;
    port_config.connected_switch_handle   = &switch_handles[1];
#endif
    status = SJA1105_PortConfigure(&switch_configs[0], &port_config);
    if (status != SJA1105_OK) return status;

    /* Switch 0 port 1 config */
#if HW_VERSION == 4
    port_config.port_num = SW0_PORT_PHY1_88Q2112;
#elif HW_VERSION == 5
    port_config.port_num = SW0_PORT_PHY4_88Q2112;
#endif
    port_config.interface     = SJA1105_INTERFACE_RGMII;
    port_config.mode          = SJA1105_MODE_MAC;
    port_config.speed         = SJA1105_SPEED_DYNAMIC;
    port_config.voltage       = SJA1105_IO_1V8;
    port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
    status                    = SJA1105_PortConfigure(&switch_configs[0], &port_config);
    if (status != SJA1105_OK) return status;

    /* Switch 0 port 2 config */
#if HW_VERSION == 4
    port_config.port_num = SW0_PORT_PHY2_88Q2112;
#elif HW_VERSION == 5
    port_config.port_num = SW0_PORT_PHY5_88Q2112;
#endif
    port_config.interface     = SJA1105_INTERFACE_RGMII;
    port_config.mode          = SJA1105_MODE_MAC;
    port_config.speed         = SJA1105_SPEED_DYNAMIC;
    port_config.voltage       = SJA1105_IO_1V8;
    port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
    status                    = SJA1105_PortConfigure(&switch_configs[0], &port_config);
    if (status != SJA1105_OK) return status;

    /* Switch 0 port 3 config */
#if HW_VERSION == 4
    port_config.port_num = SW0_PORT_PHY3_LAN8671;
#elif HW_VERSION == 5
    port_config.port_num = SW0_PORT_PHY6_LAN8671;
#endif
    port_config.interface = SJA1105_INTERFACE_RMII;
    port_config.mode      = SJA1105_MODE_MAC;
#if HW_VERSION == 4
    port_config.speed = SJA1105_SPEED_MBPS_TO_ENUM(PORT3_SPEED_MBPS);
#elif HW_VERSION == 5
    port_config.speed = SJA1105_SPEED_MBPS_TO_ENUM(PORT6_SPEED_MBPS);
#endif
    port_config.voltage            = SJA1105_IO_3V3;
    port_config.output_rmii_refclk = true;
    port_config.rx_error_unused    = false;
    status                         = SJA1105_PortConfigure(&switch_configs[0], &port_config);
    if (status != SJA1105_OK) return status;

    /* Switch 0 port 4 config */
    port_config.port_num  = SW0_PORT_HOST;
    port_config.interface = SJA1105_INTERFACE_RMII;
    port_config.mode      = SJA1105_MODE_PHY;
#if HW_VERSION == 4
    port_config.speed = SJA1105_SPEED_MBPS_TO_ENUM(PORT4_SPEED_MBPS);
#elif HW_VERSION == 5
    port_config.speed = SJA1105_SPEED_MBPS_TO_ENUM(PORT7_SPEED_MBPS);
#endif
    port_config.voltage            = SJA1105_IO_3V3;
    port_config.output_rmii_refclk = true;
    port_config.rx_error_unused    = true;
    status                         = SJA1105_PortConfigure(&switch_configs[0], &port_config);
    if (status != SJA1105_OK) return status;


#if HW_VERSION == 5

    /* Switch 1 port 0 config */
    port_config.port_num      = SW1_PORT_PHY0_DP83867;
    port_config.interface     = SJA1105_INTERFACE_RGMII;
    port_config.mode          = SJA1105_MODE_MAC;
    port_config.speed         = SJA1105_SPEED_DYNAMIC;
    port_config.voltage       = SJA1105_IO_1V8;
    port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
    status                    = SJA1105_PortConfigure(&switch_configs[1], &port_config);
    if (status != SJA1105_OK) return status;

    /* Switch 1 port 1 config */
    port_config.port_num      = SW1_PORT_PHY1_88Q2112;
    port_config.interface     = SJA1105_INTERFACE_RGMII;
    port_config.mode          = SJA1105_MODE_MAC;
    port_config.speed         = SJA1105_SPEED_DYNAMIC;
    port_config.voltage       = SJA1105_IO_1V8;
    port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
    status                    = SJA1105_PortConfigure(&switch_configs[1], &port_config);
    if (status != SJA1105_OK) return status;

    /* Switch 1 port 2 config */
    port_config.port_num      = SW1_PORT_PHY2_88Q2112;
    port_config.interface     = SJA1105_INTERFACE_RGMII;
    port_config.mode          = SJA1105_MODE_MAC;
    port_config.speed         = SJA1105_SPEED_DYNAMIC;
    port_config.voltage       = SJA1105_IO_1V8;
    port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
    status                    = SJA1105_PortConfigure(&switch_configs[1], &port_config);
    if (status != SJA1105_OK) return status;

    /* Switch 1 port 3 config */
    port_config.port_num      = SW1_PORT_PHY3_88Q2112;
    port_config.interface     = SJA1105_INTERFACE_RGMII;
    port_config.mode          = SJA1105_MODE_MAC;
    port_config.speed         = SJA1105_SPEED_DYNAMIC;
    port_config.voltage       = SJA1105_IO_1V8;
    port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
    status                    = SJA1105_PortConfigure(&switch_configs[1], &port_config);
    if (status != SJA1105_OK) return status;

    /* Switch 1 port 4 config */
    port_config.port_num                  = SW1_PORT_SW0;
    port_config.interface                 = SJA1105_INTERFACE_RGMII;
    port_config.mode                      = SJA1105_MODE_MAC;
    port_config.speed                     = SJA1105_SPEED_1G;
    port_config.voltage                   = SJA1105_IO_1V8;
    port_config.rgmii_id_mode             = SJA1105_RGMII_ID_NONE; /* Delays handled by switch 0 */
    port_config.connected_switch_port_num = SW0_PORT_SW1;
    port_config.connected_switch_handle   = &switch_handles[0];
    status                                = SJA1105_PortConfigure(&switch_configs[1], &port_config);
    if (status != SJA1105_OK) return status;

#endif

    /* Initialise the switches with the default config(s) */
    for (switch_index_t i = 0; i < NUM_SWITCHES; i++) {
        status = SJA1105_Init(
            &switch_handles[i],
            &switch_configs[i],
            &sja1105_callbacks,
            &switch_info[i],
            switch_fixed_length_table_buffers[i],
            switch_static_configs[i],
            switch_static_config_sizes[i]);
        if (status != SJA1105_OK) return status;
    }

    /* Disable port forwarding. This will be re-enabled as PHY's links go up
     * Also set the speed of the dynamic ports to their default values */
    for (phy_index_t i = 0; i < NUM_PHYS; i++) {
        status = switch_disable_forwarding(i);
        if (status != SJA1105_OK) return status;
        status = switch_update_speed(i, port_speeds[i]);
        if (status != SJA1105_OK) return status;
    }

#if HW_VERSION == 5
    status = SJA1105_SyncTimestamps(&switch_handles[0], &switch_handles[1]);
    if (status != SJA1105_OK) return status;
#endif

    return status;
}
