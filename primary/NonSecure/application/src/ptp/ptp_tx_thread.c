/*
 * ptp_tx_thread.c
 *
 *  Created on: May 18, 2026
 *      Author: bens1
 */

#include "assert.h"

#include "nx_stm32_eth_driver.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "utils.h"
#include "ptp_thread.h"
#include "ptp_init.h"
#include "ptp_utils.h"
#include "switch_utils.h"


TX_THREAD ptp_tx_thread_handle;
uint8_t   ptp_tx_thread_stack[PTP_TX_THREAD_STACK_SIZE];

TX_QUEUE ptp_tx_queue_handle;
uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];

TX_EVENT_FLAGS_GROUP ptp_tx_events_handle;


static volatile NX_PACKET   *ptp_tx_packet = NULL; /* Packet pointer of management frame */
static volatile port_index_t ptp_tx_port;          /* Port the management frame will be sent through */
static volatile uint8_t      ptp_tx_send_count;    /* How many switches the management frame is required to pass through */
static volatile uint8_t      tx_send_count;        /* How many switches the management frame has passed through */


static sja1105_status_t ptp_tx_mgmt_route_freed(sja1105_handle_t *dev, sja1105_mgmt_free_t reason, void *context) {

    sja1105_status_t status = SJA1105_OK;
    port_index_t     port   = (port_index_t) context;

    /* An error has occured */
    if (reason != SJA1105_MGMT_FREE_USED) {
        LOG_ERROR("PTP TX Management route freed with code %d", reason);
        error_handler();
    }

    /* The route freed is the one expected */
    if (port == ptp_tx_port) {

        /* This is the last switch in the chain, get the timestamp */
        if (dev->config->switch_id == (ptp_tx_send_count - 1)) {

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
                LOG_WARNING("PTP TX Missed a timestamp on port %d", port);
            } else {
                ptp_event_counters.tx_timestamps_received[port]++;
            }

            /* Send the transmit timestamp to the clock sync thread */
            if (port == PORT_HOST) {
                ptp_packet_event_info_t event_info;
                event_info.event = PTP_CLOCK_EVENT_TX_SWITCH_TIMESTAMP;
                event_info.time  = timestamp;
                event_info.port  = PORT_HOST;
                if (tx_queue_send(&ptp_clock_queue_handle, &event_info, TX_NO_WAIT) != TX_SUCCESS) error_handler();
            }

            /* Call PTP client notification callback */
            else if (port < NUM_PHYS) {
                nx_ptp_client_packet_timestamp_notify(&ptp_client[port], packet_ptr, &timestamp);
            }

            else {
                error_handler();
            }
        }

        /* Increment the sent counter */
        tx_send_count++;
        assert(tx_send_count <= NUM_SWITCHES);

        /* PTP Packet fully sent, let the tx thread know */
        if (tx_send_count == ptp_tx_send_count) {
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

    ptp_packet_event_info_t event_info;
    uint32_t                event_flags;

    uint8_t  depth    = 0;
    uint32_t attempts = 0;

#if DEBUG
    uint32_t enqueued;
    uint32_t queue_high_water_mark = 0;
#endif

    while (1) {

        tx_status = tx_queue_receive(&ptp_tx_queue_handle, &event_info, TX_WAIT_FOREVER);
#if DEBUG
        tx_queue_info_get(&ptp_tx_queue_handle, NULL, &enqueued, NULL, NULL, NULL, NULL);
        if ((enqueued + 1) > queue_high_water_mark) {
            queue_high_water_mark = (enqueued + 1);
            LOG_INFO("PTP TX Queue high water mark = %lu/%d", queue_high_water_mark, PTP_TX_QUEUE_SIZE);
        }
#endif
        if (tx_status == NX_SUCCESS) {

            switch (event_info.event) {

                case PTP_TX_EVENT_SEND_PACKET: {

                    /* Save packet info */
                    ptp_tx_packet = event_info.packet_ptr;
                    ptp_tx_port   = event_info.port;
                    tx_send_count = 0;

                    /* Create a management route for the packet */
                    if (switch_create_mgmt_route(event_info.port, ptp_dst_addr, true, PTP_TX_TSREG, &depth, &ptp_tx_mgmt_route_freed, (void *) event_info.port) != SJA1105_OK) error_handler();
                    ptp_tx_send_count = depth; /* The number of times the route must be freed is equal to the number of switches it must pass through */

                    /* Send the packet */
                    nx_status = nx_link_raw_packet_send(&nx_ip_instance, PRIMARY_INTERFACE, event_info.packet_ptr);
                    if (nx_status != NX_SUCCESS) error_handler();
                    ptp_event_counters.tx_packets_sent[event_info.port]++;

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
                            ptp_tx_send_count = 0;
                            tx_send_count     = 0;
                            break;
                        }

                        /* Error occured */
                        else if (tx_status != TX_NO_EVENTS) {
                            error_handler();
                        }

                        /* Keep waiting */
                        attempts++;
                        if (attempts == MS_TO_TICKS(PTP_TX_TIMEOUT)) error_handler();

                    } while (1);

                    /* Wait for the ethernet driver to be done with the packet */
                    tx_status = tx_event_flags_get(
                        &ptp_tx_events_handle,
                        PTP_TX_EVENT_PACKET_FREE,
                        TX_OR_CLEAR,
                        &event_flags,
                        MS_TO_TICKS(PTP_TX_TIMEOUT));
                    if ((tx_status != TX_SUCCESS) && (tx_status != TX_NO_EVENTS)) error_handler();

                    /* Release the packet */
                    nx_link_packet_transmitted(&nx_ip_instance, PRIMARY_INTERFACE, event_info.packet_ptr, NULL);
                    ptp_tx_packet = NULL;

                    break;
                }

                default: {
                    error_handler();
                    break;
                }
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

    bool filter_packet = false;

    USHORT ether_type;
    USHORT vlan_tag;
    UCHAR  vlan_tag_valid;
    UINT   header_size;

    /* Get info from the header */
    if (nx_link_ethernet_header_parse(
            packet_ptr,
            NULL,
            NULL,
            NULL,
            NULL,
            &ether_type,
            &vlan_tag,
            &vlan_tag_valid,
            &header_size) != NX_SUCCESS) error_handler();

    /* Packet has already been filtered */
    if (packet_ptr == ptp_tx_packet) {
        filter_packet = false;
    }

    /* Packet is PTP */
    else if (ether_type == NX_LINK_ETHERNET_PTP &&
             ((vlan_tag_valid != NX_TRUE) || ((vlan_tag & NX_LINK_VLAN_ID_MASK) == PTP_VLAN))) {

        /* Queue the packet to be sent */
        filter_packet = true;

        uint8_t                 port_idx;
        port_index_t            port_number;
        ptp_packet_event_info_t event_info;

        /* Extract the port */
        port_idx    = header_size + PTP_HEADER_PORT_OFFSET;                         /* Index into the packet */
        port_number = (uint16_t) ((packet_ptr->nx_packet_prepend_ptr[port_idx] << 8) |
                                  packet_ptr->nx_packet_prepend_ptr[port_idx + 1]); /* Extract the 1-indexed port number */
        port_number--;
        assert((port_number >= 0) && (port_number < NUM_PHYS));                     // TODO: NUM_PORTS

        /* Fill the event struct */
        event_info.event      = PTP_TX_EVENT_SEND_PACKET;
        event_info.packet_ptr = packet_ptr;
        event_info.port       = port_number;

        /* Queue the event */
        if (tx_queue_send(&ptp_tx_queue_handle, &event_info, TX_NO_WAIT) != TX_SUCCESS) {
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
