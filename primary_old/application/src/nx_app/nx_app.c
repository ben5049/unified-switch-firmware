/*
 * nx_app.c
 *
 *  Created on: Aug 10, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdbool.h"
#include "nx_api.h"
#include "nxd_dhcp_client.h"
#include "nxd_ptp_client.h"
#include "nx_stm32_eth_driver.h"
#include "main.h"

#include "nx_app.h"
#include "tx_app.h"
#include "ptp_thread.h"
#include "ptp_callbacks.h"
#include "config.h"
#include "comms_thread.h"
#include "state_machine.h"


NX_IP nx_ip_instance __attribute__((section(".ETH")));

NX_PACKET_POOL nx_packet_pool __attribute__((section(".ETH")));
static uint8_t nx_packet_pool_memory[NX_APP_PACKET_POOL_SIZE] __attribute__((section(".ETH")));

NX_DHCP dhcp_client __attribute__((section(".ETH")));


/* This function should be called once in MX_NetXDuo_Init */
nx_status_t nx_setup(TX_BYTE_POOL *byte_pool) {

    tx_status_t tx_status = TX_STATUS_SUCCESS;
    nx_status_t nx_status = NX_STATUS_SUCCESS;

    uint8_t *pointer;

    /* Initialize the NetXDuo system. */
    nx_system_initialize();

    /* Create the Packet pool to be used for packet allocation,
     * If extra NX_PACKET are to be used the NX_APP_PACKET_POOL_SIZE should be increased
     */
    nx_status = nx_packet_pool_create(&nx_packet_pool, "NetXDuo App Pool", DEFAULT_PAYLOAD_SIZE, nx_packet_pool_memory, NX_APP_PACKET_POOL_SIZE);
    if (nx_status != NX_SUCCESS) return nx_status;

    /* Allocate the memory for nx_ip_instance */
    tx_status = tx_byte_allocate(byte_pool, (void **) &pointer, NX_INTERNAL_IP_THREAD_STACK_SIZE, TX_NO_WAIT);
    if (tx_status != TX_SUCCESS) return NX_STATUS_POOL_ERROR;

    /* Create the main NX_IP instance */
    nx_status = nx_ip_create(&nx_ip_instance, "NetX IP instance", NX_DEFAULT_IP_ADDRESS, NX_DEFAULT_NET_MASK, &nx_packet_pool, nx_stm32_eth_driver, pointer, NX_INTERNAL_IP_THREAD_STACK_SIZE, NX_INTERNAL_IP_THREAD_PRIORITY);
    if (nx_status != NX_SUCCESS) return nx_status;

    /* Allocate the memory for ARP */
    tx_status = tx_byte_allocate(byte_pool, (void **) &pointer, DEFAULT_ARP_CACHE_SIZE, TX_NO_WAIT);
    if (tx_status != TX_SUCCESS) return NX_STATUS_POOL_ERROR;

    /* Enable ARP and provide the ARP cache size for the IP instance */
    nx_status = nx_arp_enable(&nx_ip_instance, (void *) pointer, DEFAULT_ARP_CACHE_SIZE);
    if (nx_status != NX_SUCCESS) return nx_status;

    /* Enable ICMP */
    nx_status = nx_icmp_enable(&nx_ip_instance);
    if (nx_status != NX_SUCCESS) return nx_status;

    /* Enable TCP */
    nx_status = nx_tcp_enable(&nx_ip_instance);
    if (nx_status != NX_SUCCESS) return nx_status;

    /* Enable UDP required for DHCP communication */
    nx_status = nx_udp_enable(&nx_ip_instance);
    if (nx_status != NX_SUCCESS) return nx_status;

    /* Create the DHCP client */
    nx_status = nx_dhcp_create(&dhcp_client, &nx_ip_instance, DEVICE_NAME " DHCP client");
    if (nx_status != NX_SUCCESS) return nx_status;

    return nx_status;
}
