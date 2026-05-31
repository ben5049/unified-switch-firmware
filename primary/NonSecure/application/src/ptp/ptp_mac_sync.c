/*
 * ptp_mac_sync.c
 *
 *  Created on: May 31, 2026
 *      Author: bens1
 */

#include "app.h"
#include "utils.h"
#include "ptp_thread.h"
#include "ptp_utils.h"
#include "switch_utils.h"


#define PTP_SYNC_PAYLOAD_LENGTH (44)


typedef enum {
    MAC_SYNC_TIMESTAMP_NONE         = (0),
    MAC_SYNC_TIMESTAMP_T1_MAC_TX    = (1 << 0),
    MAC_SYNC_TIMESTAMP_T2_SWITCH_RX = (1 << 1),
    MAC_SYNC_TIMESTAMP_T3_SWITCH_TX = (1 << 2),
    MAC_SYNC_TIMESTAMP_T4_MAC_RX    = (1 << 3),
    MAC_SYNC_TIMESTAMP_ALL          = (MAC_SYNC_TIMESTAMP_T1_MAC_TX | MAC_SYNC_TIMESTAMP_T2_SWITCH_RX | MAC_SYNC_TIMESTAMP_T3_SWITCH_TX | MAC_SYNC_TIMESTAMP_T4_MAC_RX),
} mac_sync_timestamp_t;


static uint8_t dummy_sync_payload[PTP_SYNC_PAYLOAD_LENGTH] = {
    0x00 | (NX_PTP_TRANSPORT_SPECIFIC_802 << 4),                        /* 0: MsgType = 0 (Sync), transport specific = 1 (gPTP) */
    0x02,                                                               /* 1: Version = 2 */
    0x00, PTP_SYNC_PAYLOAD_LENGTH,                                      /* 2-3: Message Length = 44 bytes */
    0x00, 0x00,                                                         /* 4-5: Domain Number = 0, Reserved */
    0x02, 0x00,                                                         /* 6-7: Flags (0x02 = Two-Step flag set) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                     /* 8-15: Correction Field */
    0x00, 0x00, 0x00, 0x00,                                             /* 16-19: Reserved */
    MAC_ADDR_OCTET1, MAC_ADDR_OCTET2, MAC_ADDR_OCTET3, 0xff, 0xfe,
    MAC_ADDR_OCTET4, MAC_ADDR_OCTET5, MAC_ADDR_OCTET6, 0x00, NUM_PORTS, /* 20-29: Port Identity */
    0x00, 0x01,                                                         /* 30-31: Sequence ID */
    0x00,                                                               /* 32: Control Field (0x00 for Sync) */
    0x00,                                                               /* 33: Log Message Interval */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00          /* Sync-specific payload: Origin Timestamp (10 bytes) */
};

static volatile NX_PACKET *dummy_sync_packet_ptr = NULL; /* Packet pointer of dummy packet */


static nx_status_t ptp_create_dummy_sync(NX_PACKET **packet_ptr_ptr) {

    nx_status_t status = NX_SUCCESS;
    NX_PACKET  *packet_ptr;

    /* Allocate a packet */
    status = nx_packet_allocate(&nx_small_packet_pool, packet_ptr_ptr, NX_PHYSICAL_HEADER, NX_NO_WAIT);
    if (status != NX_SUCCESS) goto end;

    packet_ptr = *packet_ptr_ptr;

    /* Append dummy payload to make the MAC see it as a PTP packet and timestamp it */
    status = nx_packet_data_append(packet_ptr, dummy_sync_payload, sizeof(dummy_sync_payload), &nx_small_packet_pool, NX_NO_WAIT);
    if (status != NX_SUCCESS) goto cleanup;

    /* Add Ethernet header */
    status = nx_link_ethernet_header_add(&nx_ip_instance, PRIMARY_INTERFACE, packet_ptr,
                                         PTP_ETHERNET_ADDR_MSB, PTP_ETHERNET_ADDR_LSB,
                                         NX_LINK_ETHERNET_PTP);
    if (status != NX_SUCCESS) goto cleanup;

    /* Enable egress timestamping for this packet */
    packet_ptr->nx_packet_interface_capability_flag |= NX_INTERFACE_CAPABILITY_PTP_TIMESTAMP;

    goto end;

cleanup:

    if (nx_packet_release(packet_ptr) != NX_SUCCESS) error_handler();

end:

    return status;
}


nx_status_t ptp_mac_sync() {

    tx_status_t      tx_status     = TX_SUCCESS;
    nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    NX_PACKET *packet_ptr;
    uint16_t   dummy_sync_packet_length;

    ptp_packet_event_info_t event_info;

    mac_sync_timestamp_t timestamps_received;

    NX_PTP_TIME mac_tx_timestamp;    /* t1 */
    NX_PTP_TIME switch_rx_timestamp; /* t2 */
    NX_PTP_TIME switch_tx_timestamp; /* t3 */
    NX_PTP_TIME mac_rx_timestamp;    /* t4 */

    NX_PTP_TIME switch_tx_correction;
    NX_PTP_TIME offset;

    uint16_t host_speed_mbps;

    /* Create a dummy sync packet that will look convincing enough to make
     * the STM32's MAC timestamp it */
    nx_status = ptp_create_dummy_sync(&packet_ptr);
    if (nx_status != NX_SUCCESS) return nx_status;

    /* Save packet pointer for callback filtering and length for timestamp corrections */
    dummy_sync_packet_ptr    = packet_ptr;
    dummy_sync_packet_length = packet_ptr->nx_packet_length;

    /* Send the packet so that it bounces back into the host port */
    event_info.event      = PTP_TX_EVENT_SEND_PACKET;
    event_info.packet_ptr = packet_ptr;
    event_info.port       = PORT_HOST;
    tx_status             = tx_queue_send(&ptp_tx_queue_handle, &event_info, TX_NO_WAIT);
    if (tx_status != TX_SUCCESS) {

        /* Free the packet */
        nx_status = nx_packet_release(packet_ptr);
        NX_CHECK(nx_status);
        dummy_sync_packet_ptr = NULL;

        DEBUG_STOP();
        return nx_status;
    }

    /* Gather up all the timestamps */
    timestamps_received = MAC_SYNC_TIMESTAMP_NONE;
    while (timestamps_received != MAC_SYNC_TIMESTAMP_ALL) {

        /* Receive timestamp from the queue */
        tx_status = tx_queue_receive(&ptp_clock_queue_handle, &event_info, MS_TO_TICKS(PTP_CLOCK_TIMEOUT));
        if (tx_status == TX_QUEUE_EMPTY) {
            break;
        } else {
            TX_CHECK(tx_status);
        }

        assert(event_info.port == PORT_HOST);

        switch (event_info.event) {
            case PTP_CLOCK_EVENT_TX_MAC_TIMESTAMP: {
                assert(!(timestamps_received & MAC_SYNC_TIMESTAMP_T1_MAC_TX)); /* Timestamp shouldn't be received twice */
                mac_tx_timestamp     = event_info.time;
                timestamps_received |= MAC_SYNC_TIMESTAMP_T1_MAC_TX;
                break;
            }
            case PTP_CLOCK_EVENT_RX_SWITCH_TIMESTAMP: {
                assert(!(timestamps_received & MAC_SYNC_TIMESTAMP_T2_SWITCH_RX)); /* Timestamp shouldn't be received twice */
                switch_rx_timestamp  = event_info.time;
                timestamps_received |= MAC_SYNC_TIMESTAMP_T2_SWITCH_RX;
                break;
            }
            case PTP_CLOCK_EVENT_TX_SWITCH_TIMESTAMP: {
                assert(!(timestamps_received & MAC_SYNC_TIMESTAMP_T3_SWITCH_TX)); /* Timestamp shouldn't be received twice */
                switch_tx_timestamp  = event_info.time;
                timestamps_received |= MAC_SYNC_TIMESTAMP_T3_SWITCH_TX;
                break;
            }
            case PTP_CLOCK_EVENT_RX_MAC_TIMESTAMP: {
                assert(!(timestamps_received & MAC_SYNC_TIMESTAMP_T4_MAC_RX)); /* Timestamp shouldn't be received twice */
                mac_rx_timestamp     = event_info.time;
                timestamps_received |= MAC_SYNC_TIMESTAMP_T4_MAC_RX;
                break;
            }
            default:
                error_handler();
                break;
        }
    }

    /* Note: The TX and RX threads handle releasing packets */
    dummy_sync_packet_ptr = NULL;

    /* Exit if missing timestamps */
    if (timestamps_received != MAC_SYNC_TIMESTAMP_ALL) {
        LOG_WARNING("MAC Sync failed: missing timestamps (received 0x%01x)", timestamps_received);
        ptp_event_counters.mac_sync_failed++;
        return nx_status;
    }

    /* The switch's egress timestamp in this scenario is for the start of
     * the META frame so we must subtract the time it took to send the PTP
     * packet and the time between frames (AH1704 p65).
     *
     * Note: An Ethernet packet cannot be shorter than 64 bytes.
     */
    switch_status = switch_get_speed(PORT_HOST, &host_speed_mbps);
    SWITCH_CHECK(switch_status);
    switch_tx_correction.nanosecond  = (MAX(dummy_sync_packet_length, 64) + SJA1105_IFG + SJA1105_PREAMBLE) * 8 * MHZ_TO_NS(host_speed_mbps);
    switch_tx_correction.second_low  = 0;
    switch_tx_correction.second_high = 0;
    _nx_ptp_client_utility_time_diff(&switch_tx_timestamp, &switch_tx_correction, &switch_tx_timestamp);

    /* Calculate the offset between local (MAC) and remote (switch) clocks
     * for both the TX and RX path
     *
     * Note: This eliminates the approximately 1000ns of combined ingress
     *       and egress latency from the SJA1105, assuming they are equal.
     */
    ptp_compute_offset(&mac_tx_timestamp, &switch_rx_timestamp, &switch_tx_timestamp, &mac_rx_timestamp, &offset);

    /* Adjust time */
    if ((offset.second_high == 0) && (offset.second_low == 0)) {

        /* Coarse adjustment */
        if (ABS(offset.nanosecond) > MS_TO_NS(100)) {
            ptp_mac_adjust_time_coarse(&offset);
        }

        /* Fine adjustment */
        else {
            // TODO: control loop instead
            ptp_mac_adjust_time_coarse(&offset);
        }
    }

    /* Set time */
    else {
        ptp_mac_set_time(&switch_rx_timestamp);
    }

    return nx_status;
}


uint8_t ptp_tx_timestamp_filter_packet(NX_PACKET *packet_ptr, NX_PTP_TIME *timestamp) {

    tx_status_t             status = TX_SUCCESS;
    bool                    filter = false;
    ptp_packet_event_info_t event_info;

    if ((dummy_sync_packet_ptr != NULL) && (packet_ptr == dummy_sync_packet_ptr)) {

        filter = true;

        event_info.event = PTP_CLOCK_EVENT_TX_MAC_TIMESTAMP;
        event_info.time  = *timestamp;
        event_info.port  = PORT_HOST;
        status           = tx_queue_send(&ptp_clock_queue_handle, &event_info, TX_NO_WAIT);
        TX_CHECK(status);
    }

    return filter;
}
