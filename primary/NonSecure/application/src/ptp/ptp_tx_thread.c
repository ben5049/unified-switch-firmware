/*
 * ptp_tx_thread.c
 *
 *  Created on: May, 2026
 *      Author: bens1
 */

#include "nx_stm32_eth_driver.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "utils.h"
#include "ptp_thread.h"
#include "switch_utils.h"


#define PTP_EVENT_PACKET_TX (127) /* Random number to not conflict with the NX_PTP_CLIENT_EVENTs */


TX_THREAD ptp_tx_thread_handle;
uint8_t   ptp_tx_thread_stack[PTP_TX_THREAD_STACK_SIZE];

TX_QUEUE ptp_tx_queue_handle;
uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

TX_SEMAPHORE ptp_tx_semaphore_handle;

static const uint8_t ptp_dst_addr[MAC_ADDR_SIZE] = {
    (uint8_t) (PTP_ETHERNET_ADDR_MSB >> 8),  /* Index 0: 0x01 */
    (uint8_t) (PTP_ETHERNET_ADDR_MSB),       /* Index 1: 0x80 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 24), /* Index 2: 0xC2 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 16), /* Index 3: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 8),  /* Index 4: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB)        /* Index 5: 0x0E */
};

volatile NX_PACKET *tx_packet = NULL; /* Volatile to prevent other threads holding stale copies when the come to filter */


void ptp_tx_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status = TX_SUCCESS;
    nx_status_t nx_status = NX_SUCCESS;

    ptp_event_t event;

    while (1) {

        tx_status = tx_queue_receive(&ptp_tx_queue_handle, &event, TX_WAIT_FOREVER);
        if (tx_status == NX_SUCCESS) {

            switch (event.event) {

                case PTP_EVENT_PACKET_TX: {

                    /* Get permission to transmit */
                    tx_status = tx_semaphore_get(&ptp_tx_semaphore_handle, TX_WAIT_FOREVER);
                    if (tx_status != TX_SUCCESS) error_handler();

                    /* Save packet */
                    tx_packet = (NX_PACKET *) event.event_data;

                    /* Create a management route for the packet */
                    if (switch_create_mgmt_route(event.port, ptp_dst_addr, true, false) != SJA1105_OK) error_handler();

                    /* Send the packet */
                    nx_status = nx_link_raw_packet_send(&nx_ip_instance, PRIMARY_INTERFACE, (NX_PACKET *) tx_packet);
                    if (nx_status != NX_SUCCESS) error_handler();

                    /* Clear packet */
                    tx_packet = NULL;
                    break;
                }

                default:
                    error_handler();
                    break;
            }

        } else {
            error_handler();
        }
    }
}


/* Filter out raw PTP packets that are about to be sent by the ethernet driver.
 * The application needs to deal with these packets to ensure they are sent out
 * ot the correct port. This function returns true if the packet was filtered
 * and shouldn't be sent */
uint8_t ptp_tx_filter_packet(NX_PACKET *packet_ptr) {

    bool     filter_packet = false;
    uint16_t ether_type;
    uint8_t *frame              = packet_ptr->nx_packet_prepend_ptr;
    uint16_t ptp_payload_offset = ETHERNET_PACKET_PAYLOAD_OFFSET;

    ether_type = (uint16_t) ((frame[ETHERNET_PACKET_TYPE_OFFSET] << 8) |
                             frame[ETHERNET_PACKET_TYPE_OFFSET + 1]);

    /* Packet has already been filtered */
    if (packet_ptr == tx_packet) {
        filter_packet = false;
    }

    /* Packet is PTP */
    else if (ether_type == NX_LINK_ETHERNET_PTP) {
        filter_packet = true;
    }

    /* Packet is VLAN tagged, could still be PTP */
    else if (ether_type == NX_LINK_ETHERNET_TPID) {
        ether_type = (uint16_t) ((frame[ETHERNET_PACKET_TYPE_OFFSET_VLAN] << 8) |
                                 frame[ETHERNET_PACKET_TYPE_OFFSET_VLAN + 1]);
        if (ether_type == NX_LINK_ETHERNET_PTP) {
            filter_packet      = true;
            ptp_payload_offset = ETHERNET_PACKET_PAYLOAD_OFFSET_VLAN;
        }
    }

    /* Queue the packet to be sent */
    if (filter_packet) {

        uint8_t     port_idx;
        phy_index_t port_number;
        ptp_event_t event;

        /* Extract the port */
        port_idx    = ptp_payload_offset + PTP_HEADER_PORT_OFFSET;               /* Index into the packet */
        port_number = (uint16_t) ((frame[port_idx] << 8) | frame[port_idx + 1]); /* Extract the 1-indexed port number */
        port_number--;
        assert((port_number >= 0) && (port_number < NUM_PHYS));

        /* Fill the event struct */
        event.event      = PTP_EVENT_PACKET_TX;
        event.event_data = packet_ptr;
        event.port       = port_number;

        /* Queue the event */
        if (tx_queue_send(&ptp_tx_queue_handle, &event, TX_NO_WAIT) != TX_SUCCESS) {
#if DEBUG
            error_handler();
#endif

            /* Queue is full, release the packet instead */
            // TODO: increment a dropped packet counter
            if (nx_packet_release(packet_ptr) != NX_SUCCESS) error_handler();
        }
    }

    return filter_packet;
}
