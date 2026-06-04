/*
 * ptp_rx_thread.c
 *
 *  Created on: May 20, 2026
 *      Author: bens1
 */

#include "assert.h"

#include "nx_stm32_eth_driver.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "utils.h"
#include "ptp.h"
#include "switch_utils.h"


TX_THREAD ptp_rx_thread_handle;
uint8_t   ptp_rx_thread_stack[PTP_RX_THREAD_STACK_SIZE];

TX_QUEUE ptp_rx_packet_queue_handle;
uint32_t ptp_rx_packet_queue_stack[PTP_RX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];
TX_QUEUE ptp_rx_meta_queue_handle;
uint32_t ptp_rx_meta_queue_stack[PTP_RX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];

static volatile uint32_t meta_debt = 0;


static inline bool ptp_is_event_packet(uint8_t *payload) {

    /* First byte contains the message type and transport specific */
    ptp_message_type_t message_type = payload[0] & PTP_MESSAGE_TYPE_MASK;

    return ((message_type == PTP_MESSAGE_TYPE_SYNC) ||
            (message_type == PTP_MESSAGE_TYPE_PDELAY_REQ) ||
            (message_type == PTP_MESSAGE_TYPE_PDELAY_RESP));
}


static inline tx_status_t ptp_flush_rx_queues() {

    tx_status_t tx_status = TX_SUCCESS;

    tx_status = ptp_flush_packet_queue(&ptp_rx_packet_queue_handle);
    if (tx_status != TX_SUCCESS) return tx_status;
    tx_status = ptp_flush_packet_queue(&ptp_rx_meta_queue_handle);
    if (tx_status != TX_SUCCESS) return tx_status;

    /* Clear any pending debt since the queues have been completely reset */
    meta_debt = 0;

    return tx_status;
}


void ptp_rx_thread_entry(uint32_t initial_input) {

    tx_status_t      tx_status     = TX_SUCCESS;
    nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    ptp_packet_event_info_t event_info;
    ptp_event_t             event;

    NX_PACKET   *ptp_packet;
    NX_PTP_TIME  timestamp;
    port_index_t port;
    UINT         header_size;
    bool         event_packet;

#if DEBUG
    uint32_t enqueued;
    uint32_t queue_high_water_mark = 0;
#endif

    while (1) {

        tx_status = tx_queue_receive(&ptp_rx_packet_queue_handle, &event_info, TX_WAIT_FOREVER);
        TX_CHECK(tx_status);

#if DEBUG

        tx_status = tx_queue_info_get(&ptp_rx_packet_queue_handle, NULL, &enqueued, NULL, NULL, NULL, NULL);
        TX_CHECK(tx_status);
        if ((enqueued + 1) > queue_high_water_mark) {
            queue_high_water_mark = (enqueued + 1);
            LOG_INFO("PTP: RX Queue high water mark = %lu/%d", queue_high_water_mark, PTP_RX_QUEUE_SIZE);
        }

#endif

        event      = event_info.event;
        ptp_packet = event_info.packet_ptr;

        assert(event == PTP_RX_EVENT_RECEIVE_PACKET);

        /* Get the follow up META frame */
        tx_status = tx_queue_receive(&ptp_rx_meta_queue_handle, &event_info, MS_TO_TICKS(PTP_RX_TIMEOUT));

        /* META frame lost, drop the PTP packet since it cannot be timestamped.
         * Also flush the queues to prevent alignment issues. */
        if (tx_status != TX_SUCCESS) {

            nx_status = nx_packet_release(ptp_packet);
            NX_CHECK(nx_status);

            /* Flush both queues */
            ptp_flush_rx_queues();

            ptp_event_counters.rx_no_meta++;
            LOG_ERROR("PTP: RX No META frame found");
            continue;
        }

        /* Parse information from the PTP packet header
         * Note: The source and destination addresses have been overwritten by
         *       timestamps. */
        nx_status = nx_link_ethernet_header_parse(ptp_packet, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &header_size);
        NX_CHECK(nx_status);
        event_packet = ptp_is_event_packet(ptp_packet->nx_packet_prepend_ptr + header_size);
        if (event_packet) {
            ptp_event_counters.rx_events++;
        } else {
            ptp_event_counters.rx_general++;
        }
        ptp_event_counters.rx_port[port]++;

        /* Parse META frame for timestamp and port then free it */
        switch_status = switch_parse_and_free_meta_frame(
            event_info.packet_ptr,
            event_packet,
            &port,
            &timestamp);
        SWITCH_CHECK(switch_status);

        /* Packet was sent by us to synchronise switch clock with
         * STM32 MAC clock */
        if (port == PORT_HOST) {

            /* Send switch timestamp */
            event_info.event = PTP_CLOCK_EVENT_RX_SWITCH_TIMESTAMP;
            event_info.time  = timestamp;
            event_info.port  = PORT_HOST;
            tx_status        = tx_queue_send(&ptp_mac_sync_queue_handle, &event_info, TX_NO_WAIT);
            TX_CHECK(tx_status);

            /* Send MAC timestamp */
            event_info.event = PTP_CLOCK_EVENT_RX_MAC_TIMESTAMP;
            ptp_packet_extract_timestamp(ptp_packet, &event_info.time);
            event_info.port = PORT_HOST;
            tx_status       = tx_queue_send(&ptp_mac_sync_queue_handle, &event_info, TX_NO_WAIT);
            TX_CHECK(tx_status);

            /* Release the packet */
            nx_status = nx_packet_release(ptp_packet);
            NX_CHECK(nx_status);
        }

        /* Packet was sent by an external device */
        else if (port < NUM_PHYS) {

            /* Store the resulting timestamp in the packet */
            ptp_packet_insert_timestamp(ptp_packet, &timestamp);

            /* Notify the client
             * Note: Only the packet pointer, header size and client
             *       pointer are used by this function. If this changes
             *       then the other fields must be extracted from the
             *       header, rather than the static values used here. */
            nx_status = _nx_ptp_client_ethernet_receive_notify(
                &nx_ip_instance,
                PRIMARY_INTERFACE,
                ptp_packet,
                PTP_ETHERNET_ADDR_MSW,
                PTP_ETHERNET_ADDR_LSW,
                NX_LINK_ETHERNET_PTP,
                header_size,
                &ptp_client[port], NULL);
            NX_CHECK(nx_status);
        }

        else {
            error_handler();
        }
    }
}


/* Send packets straight to the application if required. Returns true if packet has been dealt with and the caller should ignore */
uint8_t ptp_rx_filter_packet(NX_PACKET *packet_ptr, uint32_t ts[2]) {

    tx_status_t tx_status = TX_SUCCESS;
    nx_status_t nx_status = NX_SUCCESS;

    bool filter_packet = false;

    bool meta_frame;
    bool management_frame;
    bool ptp_frame;

    ptp_packet_event_info_t event_info;

    ULONG  dst_msw, dst_lsw;
    ULONG  src_msw, src_lsw;
    USHORT ether_type;
    USHORT vlan_tag;
    UCHAR  vlan_tag_valid;
    UINT   header_size;

    /* If PTP hasn't been started don't filter */
    if (!ptp_initialised) return filter_packet;

    /* Get info from the header */
    nx_status = nx_link_ethernet_header_parse(
        packet_ptr,
        &dst_msw,
        &dst_lsw,
        &src_msw,
        &src_lsw,
        &ether_type,
        &vlan_tag,
        &vlan_tag_valid,
        &header_size);
    NX_CHECK(nx_status);

    /* Get information about the frame */
    meta_frame       = (src_msw == ptp_srcmeta_msw) && (src_lsw == ptp_srcmeta_lsw);
    management_frame = ((dst_msw & ptp_mac_flt_msw) == ptp_mac_fltres_msw) &&
                       ((dst_lsw & ptp_mac_flt_lsw) == ptp_mac_fltres_lsw);
    ptp_frame = ether_type == NX_LINK_ETHERNET_PTP;

    /* Discard invalid frames */
    if ((meta_frame && (meta_debt > 0)) ||  /* META frame must be consumed to prevent desync between queues */
        (meta_frame && management_frame) || /* Misconfiguration of switch */
        (meta_frame && ptp_frame) ||        /* Should be impossible, META frame type is just the length */
        (management_frame && !ptp_frame) || /* Using gPTP address but not gPTP traffic */
        (ptp_frame && !(management_frame))  /* PTP frame but not gPTP e.g. (01:1b:19:00:00:00) */
    ) {

        filter_packet = true;

        /* Release the packet */
        nx_status = nx_packet_release(packet_ptr);
        NX_CHECK(nx_status);

        /* Invalid management frame will have an invalid meta frame */
        if (management_frame) {
            meta_debt++;
        }

        /* META Frame consumed to prevent desync */
        else if (meta_frame && (meta_debt > 0)) {
            meta_debt--;
        }
    }

    /* Pass valid frames */
    else if (meta_frame || (management_frame && ptp_frame)) {

        /* Filter all frames regardless of VLAN */
        filter_packet = true;

        /* Only pass packets with the correct VLAN to the rx thread */
        if ((vlan_tag_valid != NX_TRUE) || ((vlan_tag & NX_LINK_VLAN_ID_MASK) == PTP_VLAN)) {

            /* META Frame */
            if (meta_frame) {

                /* Queue the packet to be sent */
                event_info.event      = PTP_RX_EVENT_RECEIVE_PACKET;
                event_info.packet_ptr = packet_ptr;
                tx_status             = tx_queue_send(&ptp_rx_meta_queue_handle, &event_info, TX_NO_WAIT);
                if (tx_status != TX_SUCCESS) {
                    DEBUG_STOP();

                    /* Queue is full, release the packet instead */
                    ptp_event_counters.rx_meta_dropped++;
                    nx_status = nx_packet_release(packet_ptr);
                    NX_CHECK(nx_status);

                    /* Flush to prevent desync */
                    tx_status = ptp_flush_rx_queues();
                    TX_CHECK(tx_status);

                } else {
                    ptp_event_counters.rx_meta++;
                }
            }

            /* PTP Packet */
            else {

                /* Store the receive timestamp at the start of the packet */
                NX_PTP_TIME timestamp;
                timestamp.nanosecond  = ts[0];
                timestamp.second_low  = ts[1];
                timestamp.second_high = 0;
                ptp_packet_insert_timestamp(packet_ptr, &timestamp);

                /* Queue the packet to be processed */
                event_info.event      = PTP_RX_EVENT_RECEIVE_PACKET;
                event_info.packet_ptr = packet_ptr;
                tx_status             = tx_queue_send(&ptp_rx_packet_queue_handle, &event_info, TX_NO_WAIT);
                if (tx_status != TX_SUCCESS) {
                    DEBUG_STOP();

                    /* Queue is full, release the packet instead */
                    ptp_event_counters.rx_packets_dropped++;
                    nx_status = nx_packet_release(packet_ptr);
                    NX_CHECK(nx_status);

                    /* Ignore the next META frame */
                    meta_debt++;
                } else {
                    ptp_event_counters.rx_packets++;
                }
            }
        }

        /* Invalid VLAN */
        else {
            DEBUG_STOP();

            ptp_event_counters.rx_invalid_vlan++;
            nx_status = nx_packet_release(packet_ptr);
            NX_CHECK(nx_status);

            /* META Frame: flush to prevent desync */
            if (meta_frame) {
                tx_status = ptp_flush_rx_queues();
                TX_CHECK(tx_status);
            }

            /* PTP Packet: ignore the next META frame */
            else {
                meta_debt++;
            }
        }
    }

    return filter_packet;
}
