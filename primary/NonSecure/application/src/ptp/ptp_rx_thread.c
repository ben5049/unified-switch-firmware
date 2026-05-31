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


void ptp_rx_thread_entry(uint32_t initial_input) {

    tx_status_t      tx_status     = TX_SUCCESS;
    nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    ptp_packet_event_info_t event_info;

    NX_PACKET   *ptp_packet;
    NX_PTP_TIME  timestamp;
    port_index_t port;
    UINT         header_size;

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

        switch (event_info.event) {

            case PTP_RX_EVENT_RECEIVE_PACKET: {

                ptp_packet = event_info.packet_ptr;

                /* Get the follow up META frame */
                tx_status = tx_queue_receive(&ptp_rx_meta_queue_handle, &event_info, MS_TO_TICKS(PTP_RX_TIMEOUT));

                /* META frame lost, drop the PTP packet since it cannot be timestamped.
                 * Also flush the queues to prevent alignment issues. */
                if (tx_status != TX_SUCCESS) {
                    nx_status = nx_packet_release(ptp_packet);
                    NX_CHECK(nx_status);

                    /* Flush both queues */
                    tx_status = ptp_flush_packet_queue(&ptp_rx_packet_queue_handle);
                    TX_CHECK(tx_status);
                    tx_status = ptp_flush_packet_queue(&ptp_rx_meta_queue_handle);
                    TX_CHECK(tx_status);

                    ptp_event_counters.rx_no_meta++;
                    LOG_ERROR("PTP RX No META frame found");
                    break;
                }

                /* Parse META frame for timestamp and port then free it */
                switch_status = switch_parse_and_free_meta_frame(
                    event_info.packet_ptr,
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

                    /* Parse information from the packet header */
                    nx_status = nx_link_ethernet_header_parse(ptp_packet, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &header_size);
                    NX_CHECK(nx_status);

                    /* Notify the client */
                    nx_status = _nx_ptp_client_ethernet_receive_notify(NULL, 0, ptp_packet, 0, 0, 0, header_size, &ptp_client[port], NULL);
                    NX_CHECK(nx_status);
                }

                else {
                    error_handler();
                }

                break;
            }

            default: {
                error_handler();
                break;
            }
        }
    }
}


/* Send packets straight to the application if required. Returns true if packet has been dealt with and the caller should ignore */
uint8_t ptp_rx_filter_packet(NX_PACKET *packet_ptr, uint32_t ts[2]) {

    tx_status_t tx_status = TX_SUCCESS;
    nx_status_t nx_status = NX_SUCCESS;

    bool filter_packet = false;

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
            LOG_ERROR("PTP RX Dropped meta frame");
            nx_status = nx_packet_release(packet_ptr);
            NX_CHECK(nx_status);
        } else {
            ptp_event_counters.rx_meta++;
        }
    }

    /* PTP packet */
    if (ether_type == NX_LINK_ETHERNET_PTP) {

        /* Filter out all PTP packets, regardless of VLAN or address */
        filter_packet = true;

        /* PTP but using different destination (non-gPTP, probably E2E broadcast address) */
        // TODO: get MAC_FLTRES & MAC_FLT and check match
        if ((((dst_lsw & 0xff0000ff) != PTP_ETHERNET_ADDR_LSB) || (dst_msw != PTP_ETHERNET_ADDR_MSB))) {
            ptp_event_counters.rx_wrong_dst++;
            nx_status = nx_packet_release(packet_ptr);
            NX_CHECK(nx_status);
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
#if DEBUG
                    error_handler();
#endif

                    /* Queue is full, release the packet instead */
                    ptp_event_counters.rx_packets_dropped++;
                    LOG_ERROR("PTP RX Dropped PTP packet");
                    nx_status = nx_packet_release(packet_ptr);
                    NX_CHECK(nx_status);
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

    return filter_packet;
}
