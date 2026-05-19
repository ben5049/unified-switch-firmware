/*
 * ptp_tx_thread.c
 *
 *  Created on: May, 2026
 *      Author: bens1
 */

#include "assert.h"

#include "nx_stm32_eth_driver.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "utils.h"
#include "ptp_thread.h"
#include "switch_utils.h"


#define PTP_TX_EVENT_MGMT_FREE   (1 << 0) /* All management routes have been used */
#define PTP_TX_EVENT_PACKET_FREE (1 << 1) /* Ethernet driver is done with packet and it can be freed */
#define PTP_TX_EVENT_SEND_PACKET (1 << 7) /* Mustn't conflict with the NX_PTP_CLIENT_EVENT macros */


TX_THREAD ptp_tx_thread_handle;
uint8_t   ptp_tx_thread_stack[PTP_TX_THREAD_STACK_SIZE];

TX_QUEUE ptp_tx_queue_handle;
uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

TX_EVENT_FLAGS_GROUP ptp_tx_events_handle;

static const uint8_t ptp_dst_addr[MAC_ADDR_SIZE] = {
    (uint8_t) (PTP_ETHERNET_ADDR_MSB >> 8),  /* Index 0: 0x01 */
    (uint8_t) (PTP_ETHERNET_ADDR_MSB),       /* Index 1: 0x80 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 24), /* Index 2: 0xC2 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 16), /* Index 3: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 8),  /* Index 4: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB)        /* Index 5: 0x0E */
};

static volatile NX_PACKET  *ptp_tx_packet = NULL;  /* Packet pointer of management frame */
static volatile phy_index_t ptp_tx_port;           /* Port the management frame will be sent through */
static volatile uint8_t     ptp_ptp_tx_send_count; /* How many switches the management frame is required to pass through */
static volatile uint8_t     tx_send_count;         /* How many switches the management frame has passed through */


static sja1105_status_t ptp_tx_mgmt_route_freed(sja1105_handle_t *dev, sja1105_mgmt_free_t reason, void *context) {

    sja1105_status_t status = SJA1105_OK;
    uint8_t          port   = (phy_index_t) context;

    /* An error has occured */
    if (reason != SJA1105_MGMT_FREE_USED) {
        LOG_ERROR("PTP TX Management route freed with code %d", reason);
        error_handler();
    }

    /* The route freed is the one expected */
    if (port == ptp_tx_port) {

        /* This is the last switch in the chain, get the timestamp */
        if (dev->config->switch_id == (ptp_ptp_tx_send_count - 1)) {

            NX_PTP_TIME timestamp;
            NX_PACKET  *packet_ptr = (NX_PACKET *) ptp_tx_packet;

            /* Get the timestamp */
            status = switch_get_egress_timestamp(port, PTP_TX_TSREG, &timestamp);
            if (status != SJA1105_OK) return status;

            /* No timestamp. Could be because frame never reached MAC egress port.
             * This could be because the PHY has no link */
            if ((timestamp.nanosecond == 0) &&
                (timestamp.second_low == 0) &&
                (timestamp.second_high == 0)) {
                ptp_event_counters.tx_timestamps_missed[port]++;
            } else {
                ptp_event_counters.tx_timestamps_received[port]++;
            }

            /* Call PTP client notification callback */
            nx_ptp_client_packet_timestamp_notify(&ptp_client[port], packet_ptr, &timestamp);
        }

        /* Increment the sent counter */
        tx_send_count++;
        assert(tx_send_count <= NUM_SWITCHES);

        /* PTP Packet fully sent, let the tx thread know */
        if (tx_send_count == ptp_ptp_tx_send_count) {
            if (tx_event_flags_set(&ptp_tx_events_handle, PTP_TX_EVENT_MGMT_FREE, TX_OR) != TX_SUCCESS) {
                status = SJA1105_ERROR;
                return status;
            }
        }
    }

    else {
        status = SJA1105_ERROR;
    }

    return status;
}


void ptp_tx_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status = TX_SUCCESS;
    nx_status_t nx_status = NX_SUCCESS;

    ptp_event_t event;
    uint32_t    event_flags;

    uint8_t  depth    = 0;
    uint32_t attempts = 0;

#if DEBUG
    uint32_t enqueued;
    uint32_t queue_high_water_mark = 0;
#endif

    while (1) {

        tx_status = tx_queue_receive(&ptp_tx_queue_handle, &event, TX_WAIT_FOREVER);
#if DEBUG
        tx_queue_info_get(&ptp_tx_queue_handle, NULL, &enqueued, NULL, NULL, NULL, NULL);
        if ((enqueued + 1) > queue_high_water_mark) {
            queue_high_water_mark = (enqueued + 1);
            LOG_INFO("PTP TX Queue high water mark = %lu/%d", queue_high_water_mark, PTP_TX_QUEUE_SIZE);
        }
#endif
        if (tx_status == NX_SUCCESS) {

            switch (event.event) {

                case PTP_TX_EVENT_SEND_PACKET: {

                    /* Save packet info */
                    ptp_tx_packet = (NX_PACKET *) event.event_data;
                    ptp_tx_port   = event.port;
                    tx_send_count = 0;

                    /* Create a management route for the packet */
                    if (switch_create_mgmt_route(event.port, ptp_dst_addr, true, PTP_TX_TSREG, &depth, &ptp_tx_mgmt_route_freed, (void *) event.port) != SJA1105_OK) error_handler();
                    ptp_ptp_tx_send_count = depth; /* The number of times the route must be freed is equal to the number of switches it must pass through */

                    /* Send the packet */
                    nx_status = nx_link_raw_packet_send(&nx_ip_instance, PRIMARY_INTERFACE, (NX_PACKET *) ptp_tx_packet);
                    if (nx_status != NX_SUCCESS) error_handler();
                    ptp_event_counters.tx_packets_sent[event.port]++;

                    /* Get confirmation the packet has been sent through the switch */
                    attempts = 0;
                    do {

                        /* Attempt to free management route. If successful this will
                         * get the egress timestamp and pass it to the PTP client too. */
                        if (switch_free_mgmt_route(depth) != SJA1105_OK) error_handler();

                        /* Attempt to get confirmation the packet has made it through the switch */
                        tx_status = tx_event_flags_get(&ptp_tx_events_handle, PTP_TX_EVENT_MGMT_FREE, TX_OR_CLEAR, &event_flags, 1);

                        /* Management route freed */
                        if (tx_status == TX_SUCCESS) {
                            ptp_ptp_tx_send_count = 0;
                            tx_send_count         = 0;
                            break;
                        }

                        /* Error occured */
                        else if (tx_status != TX_NO_EVENTS) {
                            error_handler();
                        }

                        /* Keep waiting */
                        attempts++;
                        if (attempts == MS_TO_TICKS(PTP_TX_PERMISSION_TIMEOUT)) error_handler();

                    } while (1);

                    /* Wait for the ethernet driver to be done with the packet */
                    attempts = 0;
                    do {

                        /* Attempt to get confirmation the packet has been sent and has made it through the switch */
                        tx_status = tx_event_flags_get(&ptp_tx_events_handle, PTP_TX_EVENT_PACKET_FREE, TX_OR_CLEAR, &event_flags, 10);

                        /* Ethernet driver is done with the packet, release it */
                        if (tx_status == TX_SUCCESS) {
                            nx_link_packet_transmitted(&nx_ip_instance, PRIMARY_INTERFACE, event.event_data, NULL);
                            ptp_tx_packet = NULL;
                            break;
                        }

                        /* Error occured */
                        else if (tx_status != TX_NO_EVENTS) {
                            error_handler();
                        }

                        /* Keep waiting */
                        attempts++;
                        if (attempts == MS_TO_TICKS(PTP_TX_PERMISSION_TIMEOUT / 10)) error_handler();

                    } while (1);

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
 * and shouldn't be sent. */
uint8_t ptp_tx_filter_packet_send(NX_PACKET *packet_ptr) {

    bool     filter_packet = false;
    uint16_t ether_type;
    uint8_t *frame              = packet_ptr->nx_packet_prepend_ptr;
    uint16_t ptp_payload_offset = ETHERNET_PACKET_PAYLOAD_OFFSET;

    ether_type = (uint16_t) ((frame[ETHERNET_PACKET_TYPE_OFFSET] << 8) |
                             frame[ETHERNET_PACKET_TYPE_OFFSET + 1]);

    /* Packet has already been filtered */
    if (packet_ptr == ptp_tx_packet) {
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
        event.event      = PTP_TX_EVENT_SEND_PACKET;
        event.event_data = packet_ptr;
        event.port       = port_number;

        /* Queue the event */
        if (tx_queue_send(&ptp_tx_queue_handle, &event, TX_NO_WAIT) != TX_SUCCESS) {
#if DEBUG
            error_handler();
#endif

            /* Queue is full, release the packet instead */
            ptp_event_counters.tx_packets_dropped[port_number]++;
            LOG_ERROR("PTP TX Dropped packet");
            if (nx_packet_release(packet_ptr) != NX_SUCCESS) error_handler();
        }
    }

    return filter_packet;
}


/* Filter out the packets being freed by the ethernet driver in the transmit complete callback */
uint8_t ptp_tx_filter_packet_free(NX_PACKET *packet_ptr) {
    if (packet_ptr == ptp_tx_packet) {
        if (tx_event_flags_set(&ptp_tx_events_handle, PTP_TX_EVENT_PACKET_FREE, TX_OR) != TX_SUCCESS) error_handler();
        return true;
    } else {
        return false;
    }
}
