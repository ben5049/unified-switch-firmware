/*
 * switch_init.c
 *
 *  Created on: Sep 04, 2025
 *      Author: bens1
 */

#include "stdatomic.h"

#include "main.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "sja1105.h"
#include "utils.h"

#if HW_VERSION == 4
#include "swv4_sja1105_static_config_default.h"
#elif HW_VERSION == 5
#include "swv5_0_sja1105_static_config_default.h"
#include "swv5_1_sja1105_static_config_default.h"
#endif


sja1105_handle_t        hsw0, hsw1;
static sja1105_config_t sw0_conf, sw1_conf;
static uint32_t         sw0_fixed_length_table_buffer[SJA1105_FIXED_BUFFER_SIZE] __ALIGNED(32);
static uint32_t         sw1_fixed_length_table_buffer[SJA1105_FIXED_BUFFER_SIZE] __ALIGNED(32);


sja1105_status_t switch_init(sja1105_handle_t *dev) {

    sja1105_status_t status = SJA1105_OK;
    sja1105_port_t   port_config;

    /* Reset switch handles */
    memset(&hsw0, 0, sizeof(sja1105_handle_t));
    memset(&hsw1, 0, sizeof(sja1105_handle_t));

    /* Reset the switches */
    HAL_GPIO_WritePin(SWCH_RST_GPIO_Port, SWCH_RST_Pin, GPIO_PIN_RESET);
    delay_ns(SJA1105_T_RST); /* 5us delay */
    HAL_GPIO_WritePin(SWCH_RST_GPIO_Port, SWCH_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(1);            /* 329us minimum until SPI commands can be written (SJA1105_T_RST_STARTUP_HW). Must be blocking since TX kernel hasn't started */

    /* Check SPI parameters */
#ifdef DEBUG
    if (SWCH_SPI.Init.DataSize != SPI_DATASIZE_32BIT) status = SJA1105_PARAMETER_ERROR;
    if (SWCH_SPI.Init.CLKPolarity != SPI_POLARITY_LOW) status = SJA1105_PARAMETER_ERROR;
    if (SWCH_SPI.Init.CLKPhase != SPI_PHASE_2EDGE) status = SJA1105_PARAMETER_ERROR;
    if (SWCH_SPI.Init.NSS != SPI_NSS_SOFT) status = SJA1105_PARAMETER_ERROR;
    if (SWCH_SPI.Init.FirstBit != SPI_FIRSTBIT_MSB) status = SJA1105_PARAMETER_ERROR;
    if (status != SJA1105_OK) return status;
#endif

    /* Initialise the ThreadX byte pools */
    status = switch_byte_pool_init(SWCH_POOL0 | SWCH_POOL1);
    if (status != SJA1105_OK)
        return status;

    /* Set the general switch parameters */
    sw0_conf.variant      = VARIANT_SJA1105Q;
    sw0_conf.timeout      = SWITCH_TIMEOUT_MS;
    sw0_conf.mgmt_timeout = SWITCH_MANAGMENT_ROUTE_TIMEOUT_MS;
    sw0_conf.host_port    = SW0_PORT_HOST; /* STM32 Host port */
    sw0_conf.casc_port    = SW0_PORT_SW1;  /* Port to switch 1 downstream */
    sw0_conf.skew_clocks  = true;          /* Improves EMI performance */
    sw0_conf.switch_id    = 0;

    /* Set the general switch parameters */
    sw1_conf.variant      = VARIANT_SJA1105Q;
    sw1_conf.timeout      = SWITCH_TIMEOUT_MS;
    sw1_conf.mgmt_timeout = SWITCH_MANAGMENT_ROUTE_TIMEOUT_MS;
    sw1_conf.host_port    = SW1_PORT_SW0;      /* Port to switch 0 upstream */
    sw1_conf.casc_port    = SJA1105_NUM_PORTS; /* No downstream switches */
    sw1_conf.skew_clocks  = true;              /* Improves EMI performance */
    sw1_conf.switch_id    = 1;

    {
        /* Switch 0 port 0 config */
        port_config.port_num      = SW0_PORT_SW1;
        port_config.interface     = SJA1105_INTERFACE_RGMII;
        port_config.mode          = SJA1105_MODE_MAC;
        port_config.speed         = SJA1105_SPEED_1G;
        port_config.voltage       = SJA1105_IO_1V8;
        port_config.rgmii_id_mode = SJA1105_RGMII_ID_TX_1NS; /* Both switches have a 1ns TX_CLK delay */
        status                    = SJA1105_PortConfigure(&sw0_conf, &port_config, true);
        if (status != SJA1105_OK) return status;

        /* Switch 0 port 1 config */
        port_config.port_num      = SW0_PORT_PHY4_88Q2112;
        port_config.interface     = SJA1105_INTERFACE_RGMII;
        port_config.mode          = SJA1105_MODE_MAC;
        port_config.speed         = SJA1105_SPEED_DYNAMIC;
        port_config.voltage       = SJA1105_IO_1V8;
        port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
        status                    = SJA1105_PortConfigure(&sw0_conf, &port_config, false);
        if (status != SJA1105_OK) return status;

        /* Switch 0 port 2 config */
        port_config.port_num      = SW0_PORT_PHY5_88Q2112;
        port_config.interface     = SJA1105_INTERFACE_RGMII;
        port_config.mode          = SJA1105_MODE_MAC;
        port_config.speed         = SJA1105_SPEED_DYNAMIC;
        port_config.voltage       = SJA1105_IO_1V8;
        port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
        status                    = SJA1105_PortConfigure(&sw0_conf, &port_config, false);
        if (status != SJA1105_OK) return status;

        /* Switch 0 port 3 config */
        port_config.port_num           = SW0_PORT_PHY6_LAN8671;
        port_config.interface          = SJA1105_INTERFACE_RMII;
        port_config.mode               = SJA1105_MODE_MAC;
        port_config.speed              = SJA1105_SPEED_MBPS_TO_ENUM(PORT6_SPEED_MBPS);
        port_config.voltage            = SJA1105_IO_3V3;
        port_config.output_rmii_refclk = true;
        port_config.rx_error_unused    = false;
        status                         = SJA1105_PortConfigure(&sw0_conf, &port_config, false);
        if (status != SJA1105_OK) return status;

        /* Switch 0 port 4 config */
        port_config.port_num           = SW0_PORT_HOST;
        port_config.interface          = SJA1105_INTERFACE_RMII;
        port_config.mode               = SJA1105_MODE_PHY;
        port_config.speed              = SJA1105_SPEED_MBPS_TO_ENUM(PORT7_SPEED_MBPS);
        port_config.voltage            = SJA1105_IO_3V3;
        port_config.output_rmii_refclk = true;
        port_config.rx_error_unused    = true;
        status                         = SJA1105_PortConfigure(&sw0_conf, &port_config, false);
        if (status != SJA1105_OK) return status;
    }

    {
        /* Switch 1 port 0 config */
        port_config.port_num      = SW1_PORT_PHY0_DP83867;
        port_config.interface     = SJA1105_INTERFACE_RGMII;
        port_config.mode          = SJA1105_MODE_MAC;
        port_config.speed         = SJA1105_SPEED_DYNAMIC;
        port_config.voltage       = SJA1105_IO_1V8;
        port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
        status                    = SJA1105_PortConfigure(&sw1_conf, &port_config, false);
        if (status != SJA1105_OK) return status;

        /* Switch 1 port 1 config */
        port_config.port_num      = SW1_PORT_PHY1_88Q2112;
        port_config.interface     = SJA1105_INTERFACE_RGMII;
        port_config.mode          = SJA1105_MODE_MAC;
        port_config.speed         = SJA1105_SPEED_DYNAMIC;
        port_config.voltage       = SJA1105_IO_1V8;
        port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
        status                    = SJA1105_PortConfigure(&sw1_conf, &port_config, false);
        if (status != SJA1105_OK) return status;

        /* Switch 1 port 2 config */
        port_config.port_num      = SW1_PORT_PHY2_88Q2112;
        port_config.interface     = SJA1105_INTERFACE_RGMII;
        port_config.mode          = SJA1105_MODE_MAC;
        port_config.speed         = SJA1105_SPEED_DYNAMIC;
        port_config.voltage       = SJA1105_IO_1V8;
        port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
        status                    = SJA1105_PortConfigure(&sw1_conf, &port_config, false);
        if (status != SJA1105_OK) return status;

        /* Switch 1 port 3 config */
        port_config.port_num      = SW1_PORT_PHY3_88Q2112;
        port_config.interface     = SJA1105_INTERFACE_RGMII;
        port_config.mode          = SJA1105_MODE_MAC;
        port_config.speed         = SJA1105_SPEED_DYNAMIC;
        port_config.voltage       = SJA1105_IO_1V8;
        port_config.rgmii_id_mode = SJA1105_RGMII_ID_NONE;
        status                    = SJA1105_PortConfigure(&sw1_conf, &port_config, false);
        if (status != SJA1105_OK) return status;

        /* Switch 1 port 4 config */
        port_config.port_num      = SW1_PORT_SW0;
        port_config.interface     = SJA1105_INTERFACE_RGMII;
        port_config.mode          = SJA1105_MODE_MAC;
        port_config.speed         = SJA1105_SPEED_1G;
        port_config.voltage       = SJA1105_IO_1V8;
        port_config.rgmii_id_mode = SJA1105_RGMII_ID_TX_1NS; /* Both switches have a 1ns TX_CLK delay */
        status                    = SJA1105_PortConfigure(&sw1_conf, &port_config, true);
        if (status != SJA1105_OK) return status;
    }

    /* Initialise the switches with the default config(s) */
    #if HW_VERSION == 4
    status = SJA1105_Init(&hsw0, &sw0_conf, &sja1105_callbacks, (void *) SWITCH0, sw0_fixed_length_table_buffer, swv4_sja1105_static_config_default, SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE);
    if (status != SJA1105_OK) return status;

    #elif HW_VERSION == 5
    status = SJA1105_Init(&hsw0, &sw0_conf, &sja1105_callbacks, (void *) SWITCH0, sw0_fixed_length_table_buffer, swv5_0_sja1105_static_config_default, SWV5_0_SJA1105_STATIC_CONFIG_DEFAULT_SIZE);
    if (status != SJA1105_OK) return status;
    status = SJA1105_Init(&hsw1, &sw1_conf, &sja1105_callbacks, (void *) SWITCH1, sw1_fixed_length_table_buffer, swv5_1_sja1105_static_config_default, SWV5_1_SJA1105_STATIC_CONFIG_DEFAULT_SIZE);
    if (status != SJA1105_OK) return status;
    #endif

    /* Set the speed of the dynamic ports. TODO: This should be after PHY auto-negotiaion */
    status = SJA1105_PortSetSpeed(&hsw1, SW1_PORT_PHY0_DP83867, SJA1105_SPEED_MBPS_TO_ENUM(PORT0_SPEED_MBPS));
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsw1, SW1_PORT_PHY1_88Q2112, SJA1105_SPEED_MBPS_TO_ENUM(PORT1_SPEED_MBPS));
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsw1, SW1_PORT_PHY2_88Q2112, SJA1105_SPEED_MBPS_TO_ENUM(PORT2_SPEED_MBPS));
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsw1, SW1_PORT_PHY3_88Q2112, SJA1105_SPEED_MBPS_TO_ENUM(PORT3_SPEED_MBPS));
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsw0, SW0_PORT_PHY4_88Q2112, SJA1105_SPEED_MBPS_TO_ENUM(PORT4_SPEED_MBPS));
    if (status != SJA1105_OK) return status;
    status = SJA1105_PortSetSpeed(&hsw0, SW0_PORT_PHY5_88Q2112, SJA1105_SPEED_MBPS_TO_ENUM(PORT5_SPEED_MBPS));
    if (status != SJA1105_OK) return status;

    return status;
}
