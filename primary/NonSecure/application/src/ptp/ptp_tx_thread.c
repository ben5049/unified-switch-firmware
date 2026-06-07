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
#include "ptp.h"
#include "switch.h"
#include "utils.h"
#include "validation.h"


TX_THREAD ptp_tx_thread;
uint8_t   ptp_tx_thread_stack[PTP_TX_THREAD_STACK_SIZE];

TX_QUEUE ptp_tx_queue;
uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];

TX_EVENT_FLAGS_GROUP ptp_tx_events_group;

static NX_PACKET * volatile ptp_tx_packet = NULL; /* Packet pointer of management frame */
static volatile uint8_t ptp_tx_send_count;        /* How many switches the management frame is required to pass through */
static volatile uint8_t tx_send_count;            /* How many switches the management frame has passed through */


static sja1105_status_t ptp_tx_mgmt_route_freed(sja1105_handle_t *dev, sja1105_mgmt_free_t reason, void *context) {

    tx_status_t             tx_status     = TX_SUCCESS;
    nx_status_t             nx_status     = NX_SUCCESS;
    sja1105_status_t        switch_status = SJA1105_OK;
    NX_PACKET              *packet_ptr    = (NX_PACKET *) context;
    UINT                    header_size;
    uint16_t                port;
    ptp_packet_event_info_t event_info;

    /* Forced out or timed out, could be due to STM32 driver failure */
    if (reason != SJA1105_MGMT_FREE_USED) {
        LOG_ERROR("PTP: TX Management route freed with code %d", reason);
    }

    /* The route freed is the one expected */
    if (packet_ptr == ptp_tx_packet) {

        /* Get the port */
        nx_status = nx_link_ethernet_header_parse(packet_ptr, NULL, NULL, NULL, NULL,
                                                  NULL, NULL, NULL, &header_size);
        NX_CHECK(nx_status);
        nx_status = ptp_packet_extract_port(packet_ptr, header_size, &port);
        NX_CHECK(nx_status);

        /* This is the last switch in the chain, get the timestamp */
        if (dev->config->switch_id == (ptp_tx_send_count - 1)) {

            NX_PTP_TIME timestamp  = {.second_high = 0, .second_low = 0, .nanosecond = 0};
            NX_PACKET  *packet_ptr = ptp_tx_packet;

            /* Only get timestamps for frames that were actually sent */
            if (reason == SJA1105_MGMT_FREE_USED) {

                /* Get the timestamp */
                switch_status = switch_get_egress_timestamp(port, PTP_TX_TSREG, &timestamp);
                if (switch_status != SJA1105_OK) return switch_status;

                /* No timestamp. Could be because frame never reached MAC egress port.
                 * This could be because the PHY has no link */
                if ((timestamp.nanosecond == 0) &&
                    (timestamp.second_low == 0) &&
                    (timestamp.second_high == 0)) {
                    VAL_COVER_ARRAY(PTP_TX, TS_MISSED, port);
                    LOG_WARNING("PTP: TX Missed a timestamp on port %d", port);
                } else {
                    VAL_COVER_ARRAY(PTP_TX, TS_RECEIVED, port);
                }
            }

            /* Send the transmit timestamp to the mac sync thread */
            if (port == PORT_HOST) {
                event_info.event = PTP_CLOCK_EVENT_TX_SWITCH_TIMESTAMP;
                event_info.time  = timestamp;
                event_info.port  = PORT_HOST;
                nx_status        = ptp_packet_extract_sequence_id(packet_ptr, header_size, &event_info.sequence_id);
                NX_CHECK(nx_status);
                tx_status = tx_queue_send(&ptp_mac_sync_queue, &event_info, TX_NO_WAIT);
                TX_CHECK(tx_status);
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
            tx_status = tx_event_flags_set(&ptp_tx_events_group, PTP_TX_EVENT_MGMT_FREE, TX_OR);
            TX_CHECK(tx_status);
        }
    }

    /* Unexpected management route free, do nothing */
    else {
        switch_status = SJA1105_OK;
    }

    return switch_status;
}


void ptp_tx_thread_entry(uint32_t initial_input) {

    tx_status_t      tx_status     = TX_SUCCESS;
    nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    ptp_packet_event_info_t event_info;
    uint32_t                event_flags;

    uint8_t  depth        = 0;
    uint32_t ticks_waited = 0;

#if DEBUG
    uint32_t enqueued;
    uint32_t queue_high_water_mark = 0;
#endif

    /* Thread was previously terminated without freeing packet */
    event_info.packet_ptr = ptp_tx_packet;
    if (ptp_tx_packet != NULL) nx_packet_transmit_release(event_info.packet_ptr);

    while (1) {

        tx_status = tx_queue_receive(&ptp_tx_queue, &event_info, TX_WAIT_FOREVER);
        TX_CHECK(tx_status);

#if DEBUG

        tx_status = tx_queue_info_get(&ptp_tx_queue, NULL, &enqueued, NULL, NULL, NULL, NULL);
        TX_CHECK(tx_status);
        if ((enqueued + 1) > queue_high_water_mark) {
            queue_high_water_mark = (enqueued + 1);
            LOG_INFO("PTP: TX Queue high water mark = %lu/%d", queue_high_water_mark, PTP_TX_QUEUE_SIZE);
        }

#endif

        switch (event_info.event) {

            case PTP_TX_EVENT_SEND_PACKET: {

                /* Save packet info */
                ptp_tx_packet = event_info.packet_ptr;
                tx_send_count = 0;

                /* Create a management route for the packet
                 * Note: The number of times the route must be freed is equal
                 *       to the number of switches it must pass through */
                switch_status = switch_create_mgmt_route(event_info.port,
                                                         ptp_dst_addr,
                                                         true,
                                                         PTP_TX_TSREG,
                                                         &depth,
                                                         &ptp_tx_mgmt_route_freed,
                                                         (void *) ptp_tx_packet);
                SWITCH_CHECK(switch_status);
                ptp_tx_send_count = depth;

                /* Send the packet */
                nx_status = nx_link_raw_packet_send(&nx_ip_instance,
                                                    PRIMARY_INTERFACE,
                                                    event_info.packet_ptr);

                /* Most likely Ethernet is disabled due to no ports having an external connection.
                 * Note: The driver will have freed the packet */
                if (nx_status == NX_STATUS_DRIVER_ERROR) {
                    VAL_COVER(PTP_TX, DRIVER_ERROR);
                    ptp_tx_packet = NULL;
                    switch_status = switch_purge_mgmt_routes();
                    SWITCH_CHECK(switch_status);
                    goto done;
                }

                /* Actual error */
                else if (nx_status != NX_SUCCESS) {
                    error_handler();
                }

                /* Transmit successful */
                else {
                    VAL_COVER_ARRAY(PTP_TX, PACKET_SENT, event_info.port);
                }

                /* Get confirmation the packet has been sent through the switch */
                ticks_waited = 0;
                do {

                    /* Attempt to free management route. If successful this will
                     * get the egress timestamp and pass it to the PTP client too. */
                    switch_status = switch_free_mgmt_route(depth);
                    SWITCH_CHECK(switch_status);

                    /* Attempt to get confirmation the packet has made it
                     * through the switch.
                     *
                     * Note: Despite polling over SPI frequently the packet is
                     *       almost always sent immediately so ticks_waited never
                     *       goes above 0. */
                    tx_status = tx_event_flags_get(&ptp_tx_events_group,
                                                   PTP_TX_EVENT_MGMT_FREE,
                                                   TX_OR_CLEAR,
                                                   &event_flags,
                                                   (ticks_waited < 10) ? 1 : 10);


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
                    if (ticks_waited < 10) {
                        ticks_waited++;
                    } else {
                        ticks_waited += 10;
                    }

                    /* Timeout: packet may have failed to egress switch due to
                     * destination port being down */
                    if (ticks_waited >= MS_TO_TICKS(PTP_TX_TIMEOUT)) {
                        VAL_COVER(PTP_TX, MGMT_ROUTE_TIMEOUT);
                        ptp_tx_packet = NULL;
                        switch_status = switch_purge_mgmt_routes();
                        SWITCH_CHECK(switch_status);
                        goto release;
                    }

                } while (1);

                /* Check if the transmit took longer than expected */
                if (ticks_waited > 0) {
                    VAL_COVER(PTP_TX, SLOW);
                    LOG_WARNING("PTP TX after %lu ms", TICKS_TO_MS(ticks_waited));
                }

            release:

                /* Wait for the ethernet driver to be done with the packet */
                tx_status = tx_event_flags_get(
                    &ptp_tx_events_group,
                    PTP_TX_EVENT_PACKET_FREE,
                    TX_OR_CLEAR,
                    &event_flags,
                    MS_TO_TICKS(PTP_TX_TIMEOUT));
                if ((tx_status != TX_SUCCESS) && (tx_status != TX_NO_EVENTS)) error_handler();

                /* Release the packet */
                nx_link_packet_transmitted(&nx_ip_instance,
                                           PRIMARY_INTERFACE,
                                           event_info.packet_ptr,
                                           NULL);

            done:

                ptp_tx_packet = NULL;
                break;
            }

            default: {
                error_handler();
                break;
            }
        }
    }
}


/* Filter out raw PTP packets that are about to be sent by the ethernet driver.
 * The application needs to deal with these packets to ensure they are sent out
 * ot the correct port. This function returns true if the packet was filtered
 * and shouldn't be sent. */
uint8_t ptp_tx_filter_packet_send(NX_PACKET *packet_ptr) {

    tx_status_t tx_status     = TX_SUCCESS;
    nx_status_t nx_status     = NX_SUCCESS;
    bool        filter_packet = false;

    USHORT ether_type;
    USHORT vlan_tag;
    UCHAR  vlan_tag_valid;
    UINT   header_size;

    uint16_t                port_number;
    ptp_packet_event_info_t event_info;

    /* If PTP hasn't been started don't filter */
    if (!ptp_initialised) return filter_packet;

    /* Get info from the header */
    nx_status = nx_link_ethernet_header_parse(packet_ptr, NULL, NULL, NULL,
                                              NULL, &ether_type, &vlan_tag,
                                              &vlan_tag_valid, &header_size);
    NX_CHECK(nx_status);

    /* Packet has already been filtered */
    if (packet_ptr == ptp_tx_packet) {
        filter_packet = false;
    }

    /* Packet is PTP */
    else if (ether_type == NX_LINK_ETHERNET_PTP &&
             ((vlan_tag_valid != NX_TRUE) || ((vlan_tag & NX_LINK_VLAN_ID_MASK) == PTP_VLAN))) {

        /* Take ownership of the packet */
        filter_packet = true;

        /* Get the port */
        nx_status = ptp_packet_extract_port(packet_ptr, header_size, &port_number);
        if (nx_status != NX_SUCCESS) {
            VAL_COVER(PTP_TX, PACKET_TOO_SHORT);
            nx_status = nx_packet_transmit_release(packet_ptr);
            NX_CHECK(nx_status);
            goto end;
        };

        /* Invalid port, drop packet */
        if (port_number >= NUM_PHYS) {
            VAL_COVER(PTP_TX, PACKET_INVALID_PORT);
            nx_status = nx_packet_transmit_release(packet_ptr);
            NX_CHECK(nx_status);
            goto end;
        }

        /* Remove timestamps from filtered PTP packets. Timestamps are taken by
         * the switch usually, or if MAC timestamping is needed then the packet
         * can be sent directly to the PTP transmit queue */
        packet_ptr->nx_packet_interface_capability_flag &= ~NX_INTERFACE_CAPABILITY_PTP_TIMESTAMP;

        /* Fill the event struct */
        event_info.event      = PTP_TX_EVENT_SEND_PACKET;
        event_info.packet_ptr = packet_ptr;
        event_info.port       = port_number;

        /* Queue the transmit event */
        tx_status = tx_queue_send(&ptp_tx_queue, &event_info, TX_NO_WAIT);
        if (tx_status != TX_SUCCESS) {
            VAL_TERMINATE();

            /* Queue is full, release the packet instead */
            VAL_COVER_ARRAY(PTP_TX, PACKET_DROPPED, port_number);
            nx_status = nx_packet_transmit_release(packet_ptr);
            NX_CHECK(nx_status);
            goto end;
        }
    }

end:

    return filter_packet;
}


/* Filter out the packets being freed by the ethernet driver in the transmit complete callback */
uint8_t ptp_tx_filter_packet_free(const NX_PACKET *packet_ptr) {
    if (packet_ptr == ptp_tx_packet) {
        if (tx_event_flags_set(&ptp_tx_events_group,
                               PTP_TX_EVENT_PACKET_FREE,
                               TX_OR) != TX_SUCCESS) error_handler();
        return true;
    } else {
        return false;
    }
}
