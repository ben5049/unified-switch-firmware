/*
 * ptp_clock_thread.c
 *
 *  Created on: May 26, 2026
 *      Author: bens1
 */

#include "stdint.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "nx_link.h"
#include "ptp_thread.h"
#include "utils.h"


TX_THREAD ptp_clock_thread_handle;
uint8_t   ptp_clock_thread_stack[PTP_CLOCK_THREAD_STACK_SIZE];

TX_QUEUE ptp_clock_queue_handle;
uint32_t ptp_clock_queue_stack[PTP_CLOCK_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

static volatile NX_PACKET *dummy_packet_ptr      = NULL;                    /* Packet pointer of dummy packet */
static uint8_t             dummy_packet_data[44] = {
    0x10,                                                       /* 0: MsgType = 0 (Sync), transport specific = 1 (gPTP) */
    0x02,                                                       /* 1: Version = 2 */
    0x00, 0x2c,                                                 /* 2-3: Message Length = 44 bytes */
    0x00, 0x00,                                                 /* 4-5: Domain Number = 0, Reserved */
    0x02, 0x00,                                                 /* 6-7: Flags (0x02 = Two-Step flag set) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,             /* 8-15: Correction Field */
    0x00, 0x00, 0x00, 0x00,                                     /* 16-19: Reserved */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 20-29: Port Identity */
    0x00, 0x01,                                                 /* 30-31: Sequence ID */
    0x00,                                                       /* 32: Control Field (0x00 for Sync) */
    0x00,                                                       /* 33: Log Message Interval */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* Sync-specific payload: Origin Timestamp (10 bytes) */
};


/* This thread synchronises the SJA1105 clocks with each other (if there are
 * multiple) and the STM32 timestamp clock with the main SJA1105 clock. */
void ptp_clock_thread_entry(uint32_t initial_input) {

    nx_status_t nx_status = NX_SUCCESS;
    tx_status_t tx_status = TX_SUCCESS;

    NX_PACKET       *packet_ptr;
    ptp_event_info_t event_info;

    NX_PTP_TIME mac_tx_timestamp;
    NX_PTP_TIME switch_rx_timestamp;
    NX_PTP_TIME switch_tx_timestamp;
    NX_PTP_TIME mac_rx_timestamp;

    uint8_t timestamps_received;

    while (1) {

        /* Allocate a packet */
        nx_status = nx_packet_allocate(&nx_small_packet_pool, &packet_ptr, NX_PHYSICAL_HEADER, PTP_CLOCK_TIMEOUT);
        if (nx_status != NX_SUCCESS) error_handler();

        /* Append dummy payload to make the MAC see it as a PTP packet and timestamp it */
        nx_status = nx_packet_data_append(packet_ptr, dummy_packet_data, sizeof(dummy_packet_data), &nx_small_packet_pool, NX_NO_WAIT);
        if (nx_status != NX_SUCCESS) error_handler();

        /* Add Ethernet header */
        nx_status = nx_link_ethernet_header_add(&nx_ip_instance, PRIMARY_INTERFACE, packet_ptr,
                                                PTP_ETHERNET_ADDR_MSB, PTP_ETHERNET_ADDR_LSB,
                                                NX_LINK_ETHERNET_PTP); // TODO: NX_PHYSICAL_HEADER??
        if (nx_status != NX_SUCCESS) error_handler();

        /* Enable egress timestamping for this packet */
        packet_ptr->nx_packet_interface_capability_flag |= NX_INTERFACE_CAPABILITY_PTP_TIMESTAMP;

        /* Save pointer for callback filtering */
        dummy_packet_ptr = packet_ptr;

        /* Send the packet so that it bounces back into the host port */
        event_info.event = PTP_TX_EVENT_SEND_PACKET;
        event_info.data  = packet_ptr;
        event_info.port  = PORT_HOST;
        tx_status        = tx_queue_send(&ptp_tx_queue_handle, &event_info, TX_NO_WAIT);
        if (tx_status != TX_SUCCESS) error_handler();

        /* Gather up all the timestamps */
        // TODO: add timeout
        timestamps_received = 0;
        while (timestamps_received < 4) {

            tx_status = tx_queue_receive(&ptp_clock_queue_handle, &event_info, PTP_CLOCK_TIMEOUT); // TODO: change
            if (tx_status != TX_SUCCESS) error_handler();

            assert(event_info.port == PORT_HOST);

            switch (event_info.event) {
                case PTP_CLOCK_EVENT_TX_MAC_TIMESTAMP: {
                    mac_tx_timestamp = event_info.time;
                    timestamps_received++;
                    break;
                }
                case PTP_CLOCK_EVENT_RX_SWITCH_TIMESTAMP: {
                    switch_rx_timestamp = event_info.time;
                    timestamps_received++;
                    break;
                }
                case PTP_CLOCK_EVENT_TX_SWITCH_TIMESTAMP: {
                    switch_tx_timestamp = event_info.time;
                    timestamps_received++;
                    break;
                }
                case PTP_CLOCK_EVENT_RX_MAC_TIMESTAMP: {
                    mac_rx_timestamp = event_info.time;
                    timestamps_received++;
                    break;
                }
                default:
                    error_handler();
                    break;
            }
        }

        /* Note: The TX and RX threads handle releasing packets */

        /* TODO: calculate offset between local and remote clocks in each direction */

        UNUSED(mac_tx_timestamp);
        UNUSED(switch_rx_timestamp);
        UNUSED(switch_tx_timestamp);
        UNUSED(mac_rx_timestamp);

        tx_thread_sleep_ms(1000);
    }
}


uint8_t ptp_clock_tx_filter_packet(NX_PACKET *packet_ptr, NX_PTP_TIME *timestamp) {

    bool             filter = false;
    ptp_event_info_t event_info;

    if ((dummy_packet_ptr != NULL) && (packet_ptr == dummy_packet_ptr)) {

        filter = true;

        event_info.event = PTP_CLOCK_EVENT_TX_MAC_TIMESTAMP;
        event_info.time  = *timestamp;
        event_info.port  = PORT_HOST;
        if (tx_queue_send(&ptp_clock_queue_handle, &event_info, TX_NO_WAIT) != TX_SUCCESS) error_handler();
    }

    return filter;
}
