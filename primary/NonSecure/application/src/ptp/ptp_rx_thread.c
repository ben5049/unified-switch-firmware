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
#include "ptp_thread.h"
#include "ptp_init.h"
#include "ptp_utils.h"
#include "switch_utils.h"


TX_THREAD ptp_rx_thread_handle;
uint8_t   ptp_rx_thread_stack[PTP_RX_THREAD_STACK_SIZE];

TX_QUEUE ptp_rx_packet_queue_handle;
uint32_t ptp_rx_packet_queue_stack[PTP_RX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];
TX_QUEUE ptp_rx_meta_queue_handle;
uint32_t ptp_rx_meta_queue_stack[PTP_RX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];


static inline bool ptp_is_event_packet(NX_PACKET *packet_ptr, uint32_t header_size) {

    ptp_message_type_t message_type = *(packet_ptr->nx_packet_prepend_ptr + header_size) & PTP_MESSAGE_TYPE_MASK;

    return ((message_type == PTP_MESSAGE_TYPE_SYNC) ||
            (message_type == PTP_MESSAGE_TYPE_PDELAY_REQ) ||
            (message_type == PTP_MESSAGE_TYPE_PDELAY_RESP));
}


void ptp_rx_thread_entry(uint32_t initial_input) {

    // TODO: disable if ptp not started

    tx_status_t      tx_status     = TX_SUCCESS;
    nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    ptp_packet_event_info_t event_info;
    ptp_event_t             event;

    NX_PACKET   *ptp_packet;
    NX_PACKET   *meta_frame;
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
            LOG_INFO("PTP RX Queue high water mark = %lu/%d", queue_high_water_mark, PTP_RX_QUEUE_SIZE);
        }

#endif

        event      = event_info.event;
        ptp_packet = event_info.packet_ptr;

        assert((event == PTP_RX_EVENT_RECEIVE_PACKET) || (event == PTP_RX_EVENT_RELEASE_META));

        /* Get the follow up META frame */
        tx_status = tx_queue_receive(&ptp_rx_meta_queue_handle, &event_info, MS_TO_TICKS(PTP_RX_TIMEOUT));

        /* META frame lost, drop the PTP packet since it cannot be timestamped.
         * Also flush the queues to prevent alignment issues. */
        if ((tx_status != TX_SUCCESS) && (event != PTP_RX_EVENT_RELEASE_META)) {
            nx_status = nx_packet_release(ptp_packet);
            NX_CHECK(nx_status);

            /* Flush both queues */
            tx_status = ptp_flush_packet_queue(&ptp_rx_packet_queue_handle);
            TX_CHECK(tx_status);
            tx_status = ptp_flush_packet_queue(&ptp_rx_meta_queue_handle);
            TX_CHECK(tx_status);

            ptp_event_counters.rx_no_meta++;
            LOG_ERROR("PTP RX No META frame found");
            continue;
        } else {
            meta_frame = event_info.packet_ptr;
        }

        /* Just release the META frame and continue */
        if (event == PTP_RX_EVENT_RELEASE_META) {
            if (tx_status == TX_SUCCESS) {
                nx_status = nx_packet_release(event_info.packet_ptr);
                NX_CHECK(nx_status);
            } else {
                LOG_WARNING("PTP RX: PTP_RX_EVENT_RELEASE_META with no META frame");
            }
            continue;
        }

        /* Parse information from the PTP packet header */
        nx_status = nx_link_ethernet_header_parse(ptp_packet, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &header_size);
        NX_CHECK(nx_status);
        event_packet = ptp_is_event_packet(ptp_packet, header_size);

        /* Parse META frame for timestamp and port then free it */
        switch_status = switch_parse_and_free_meta_frame(
            meta_frame,
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
                PTP_ETHERNET_ADDR_MSB,
                PTP_ETHERNET_ADDR_LSB,
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
    bool management_frame;

    ptp_packet_event_info_t event_info;

    ULONG  dst_msw, dst_lsw;
    ULONG  src_msw, src_lsw;
    USHORT ether_type;
    USHORT vlan_tag;
    UCHAR  vlan_tag_valid;
    UINT   header_size;

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

    // TODO: get MAC_FLTRES & MAC_FLT and check match
    management_frame = ((dst_lsw & 0xff0000ff) == PTP_ETHERNET_ADDR_LSB) && (dst_msw == PTP_ETHERNET_ADDR_MSB);

    /* META Frame */
    if ((src_msw == srcmeta_msw) && (src_lsw == srcmeta_lsw)) {

        /* Filter out all META frames, regardless of VLAN */
        filter_packet = true;

        // TODO: Check if META frames are VLAN tagged

        /* Queue the packet to be sent */
        event_info.event      = PTP_RX_EVENT_RECEIVE_PACKET;
        event_info.packet_ptr = packet_ptr;
        tx_status             = tx_queue_send(&ptp_rx_meta_queue_handle, &event_info, TX_NO_WAIT);
        if (tx_status != TX_SUCCESS) {
            DEBUG_STOP();

            /* Queue is full, release the packet instead */
            ptp_event_counters.rx_meta_dropped++;
            LOG_ERROR("PTP RX Dropped meta frame"); // TODO: this might hard fault?
            nx_status = nx_packet_release(packet_ptr);
            NX_CHECK(nx_status);
        } else {
            ptp_event_counters.rx_meta++;
        }
    }

    /* PTP packet */
    else if (ether_type == NX_LINK_ETHERNET_PTP) {

        /* Filter out all PTP packets, regardless of VLAN or address */
        filter_packet = true;

        /* PTP but using different destination (non-gPTP, probably E2E broadcast address) */
        if (!management_frame) {
            ptp_event_counters.rx_wrong_dst++;
            nx_status = nx_packet_release(packet_ptr);
            NX_CHECK(nx_status);
            DEBUG_STOP();
        }

        /* Correct address */
        else {

            /* Only pass packets with the correct VLAN to the rx thread */
            if ((vlan_tag_valid != NX_TRUE) || ((vlan_tag & NX_LINK_VLAN_ID_MASK) == PTP_VLAN)) {

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

                    /* Queue is full, release the packet instead */
                    ptp_event_counters.rx_packets_dropped++;
                    nx_status = nx_packet_release(packet_ptr);
                    NX_CHECK(nx_status);

                    LOG_ERROR("PTP RX Dropped PTP packet"); // TODO: this might hard fault?
                    DEBUG_STOP();

                } else {
                    ptp_event_counters.rx_packets++;
                }
            }

            /* Invalid VLAN */
            else {
                ptp_event_counters.rx_invalid_vlan++;
                nx_status = nx_packet_release(packet_ptr);
                NX_CHECK(nx_status);
            }
        }
    }

    /* Non-PTP Packet with address matching PTP switch filter. Need to pop
     * corresponding META frame so we don't get out of sync */
    else if (management_frame) {

        filter_packet = true;

        /* Put an event in the rx packet queue */
        event_info.event      = PTP_RX_EVENT_RELEASE_META;
        event_info.packet_ptr = NULL;
        tx_status             = tx_queue_send(&ptp_rx_packet_queue_handle, &event_info, TX_NO_WAIT);
        if (tx_status != TX_SUCCESS) ptp_event_counters.rx_packets_dropped++; /* Queue is full */

        /* Release the packet regardless */
        nx_status = nx_packet_release(packet_ptr);
        NX_CHECK(nx_status);
    }

    return filter_packet;
}
