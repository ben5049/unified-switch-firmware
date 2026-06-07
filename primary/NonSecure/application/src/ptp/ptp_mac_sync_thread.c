/*
 * ptp_mac_sync_thread.c
 *
 *  Created on: May 31, 2026
 *      Author: bens1
 */

#include "app.h"
#include "ptp.h"
#include "switch.h"
#include "utils.h"
#include "validation.h"


#define PTP_SYNC_PAYLOAD_LENGTH (44)


typedef enum {
    MAC_SYNC_TIMESTAMP_NONE         = (0),
    MAC_SYNC_TIMESTAMP_T1_MAC_TX    = (1 << 0),
    MAC_SYNC_TIMESTAMP_T2_SWITCH_RX = (1 << 1),
    MAC_SYNC_TIMESTAMP_T3_SWITCH_TX = (1 << 2),
    MAC_SYNC_TIMESTAMP_T4_MAC_RX    = (1 << 3),
    MAC_SYNC_TIMESTAMP_ALL          = (MAC_SYNC_TIMESTAMP_T1_MAC_TX | MAC_SYNC_TIMESTAMP_T2_SWITCH_RX | MAC_SYNC_TIMESTAMP_T3_SWITCH_TX | MAC_SYNC_TIMESTAMP_T4_MAC_RX),
} mac_sync_timestamp_t;


TX_THREAD ptp_mac_sync_thread;
uint8_t   ptp_mac_sync_thread_stack[PTP_MAC_SYNC_THREAD_STACK_SIZE];

TX_QUEUE ptp_mac_sync_queue;
uint32_t ptp_mac_sync_queue_stack[PTP_MAC_SYNC_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];

TX_EVENT_FLAGS_GROUP ptp_mac_sync_events_group;

TX_TIMER ptp_mac_sync_timer;

atomic_bool mac_synced = false;

static volatile NX_PACKET *dummy_sync_packet_ptr = NULL; /* Packet pointer of dummy packet */

static uint8_t dummy_sync_payload[PTP_SYNC_PAYLOAD_LENGTH] = {
    PTP_MESSAGE_TYPE_SYNC | (NX_PTP_TRANSPORT_SPECIFIC_802 << 4),           /* 0: MsgType = 0 (Sync), transport specific = 1 (gPTP) */
    0x02,                                                                   /* 1: Version = 2 */
    0x00, PTP_SYNC_PAYLOAD_LENGTH,                                          /* 2-3: Message Length = 44 bytes */
    0x00, 0x00,                                                             /* 4-5: Domain Number = 0, Reserved */
    0x02, 0x00,                                                             /* 6-7: Flags (0x02 = Two-Step flag set) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                         /* 8-15: Correction Field */
    0x00, 0x00, 0x00, 0x00,                                                 /* 16-19: Reserved */
    MAC_ADDR_OCTET1, MAC_ADDR_OCTET2, MAC_ADDR_OCTET3, 0xff, 0xfe,
    MAC_ADDR_OCTET4, MAC_ADDR_OCTET5, MAC_ADDR_OCTET6, 0x00, PORT_HOST + 1, /* 20-29: Port Identity (port indexed from 1) */
    0x00, 0x01,                                                             /* 30-31: Sequence ID */
    0x00,                                                                   /* 32: Control Field (0x00 for Sync) */
    0x00,                                                                   /* 33: Log Message Interval */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00              /* Sync-specific payload: Origin Timestamp (10 bytes) */
};


void ptp_mac_sync_timer_callback(uint32_t id) {
    tx_status_t status = tx_event_flags_set(&ptp_mac_sync_events_group, PTP_CLOCK_EVENT_MAC_SYNC, TX_OR);
    TX_CHECK(status);
}


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
                                         PTP_ETHERNET_ADDR_MSW, PTP_ETHERNET_ADDR_LSW,
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


/* This thread synchronises the STM32's MAC timestamp clock with the main
 * SJA1105 clock. */
void ptp_mac_sync_thread_entry(uint32_t initial_input) {

    static_assert(PTP_MAC_SYNC_CONTROLLER_KP >= 0);
    static_assert(PTP_MAC_SYNC_CONTROLLER_KI >= 0);
    static_assert(PTP_MAC_SYNC_CONTROLLER_INTEGRAL_MAX >= 0);
    static_assert(PTP_MAC_SYNC_CONTROLLER_OUTPUT_MAX <= INT32_MAX); /* For typeof(controller_output) = int32_t */

    nx_status_t      nx_status     = NX_SUCCESS;
    tx_status_t      tx_status     = TX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    NX_PACKET *packet_ptr;
    uint16_t   dummy_sync_packet_length;

    uint32_t                event_flags;
    ptp_packet_event_info_t event_info;

    mac_sync_timestamp_t timestamps_received;

    NX_PTP_TIME mac_tx_timestamp;    /* t1 */
    NX_PTP_TIME switch_rx_timestamp; /* t2 */
    NX_PTP_TIME switch_tx_timestamp; /* t3 */
    NX_PTP_TIME mac_rx_timestamp;    /* t4 */

    NX_PTP_TIME switch_tx_correction;
    NX_PTP_TIME offset;

    uint16_t host_speed_mbps;

    ptp_controller_t mac_sync_controller = {.initialised = false};
    int32_t          controller_output;
    uint32_t         addend;
    int64_t          addend_adjustment;

    /* Initialise PI controller */
    ptp_pi_controller_init(&mac_sync_controller,
                           PTP_MAC_SYNC_CONTROLLER_KP,
                           PTP_MAC_SYNC_CONTROLLER_KI,
                           PTP_MAC_SYNC_CONTROLLER_INTEGRAL_MAX,
                           tx_time_get_ms());

    /* Start the timer */
    tx_status = tx_timer_activate(&ptp_mac_sync_timer);
    TX_CHECK(tx_status);

    while (1) {

        /* Wait for an event from the timer */
        tx_status = tx_event_flags_get(
            &ptp_mac_sync_events_group,
            PTP_CLOCK_EVENT_MAC_SYNC | PTP_CLOCK_EVENT_RESET,
            TX_OR_CLEAR,
            &event_flags,
            TX_WAIT_FOREVER);
        TX_CHECK(tx_status);

        /* Reset the controller and MAC rate */
        if (event_flags & PTP_CLOCK_EVENT_RESET) {
            ptp_pi_controller_clear(&mac_sync_controller, tx_time_get_ms());
            ptp_mac_set_addend(PTP_COUNTER_ADDEND_DEFAULT);
        }

        /* Sync the MAC */
        if (event_flags & PTP_CLOCK_EVENT_MAC_SYNC) {

            /* If PTP hasn't been started don't send a packet */
            if (!ptp_initialised) continue;

            /* Create a dummy sync packet that will look convincing enough to make
             * the STM32's MAC timestamp it */
            nx_status = ptp_create_dummy_sync(&packet_ptr);
            NX_CHECK(nx_status);

            /* Save packet pointer for callback filtering and length for timestamp corrections */
            dummy_sync_packet_ptr    = packet_ptr;
            dummy_sync_packet_length = packet_ptr->nx_packet_length;

            /* Flush the timestamp queue so we don't receive any from previous failed syncs */
            tx_status = tx_queue_flush(&ptp_mac_sync_queue);
            TX_CHECK(tx_status);

            /* Send the packet so that it bounces back into the host port */
            event_info.event      = PTP_TX_EVENT_SEND_PACKET;
            event_info.packet_ptr = packet_ptr;
            event_info.port       = PORT_HOST;
            tx_status             = tx_queue_send(&ptp_tx_queue, &event_info, TX_NO_WAIT);
            if (tx_status != TX_SUCCESS) {

                /* Free the packet */
                nx_status = nx_packet_release(packet_ptr);
                NX_CHECK(nx_status);
                dummy_sync_packet_ptr = NULL;

                VAL_TERMINATE();
                continue;
            }

            /* Gather up all the timestamps */
            timestamps_received = MAC_SYNC_TIMESTAMP_NONE;
            while (timestamps_received != MAC_SYNC_TIMESTAMP_ALL) {

                /* Receive timestamp from the queue */
                tx_status = tx_queue_receive(&ptp_mac_sync_queue, &event_info, MS_TO_TICKS(PTP_CLOCK_TIMEOUT));
                if (tx_status == TX_QUEUE_EMPTY) {
                    break;
                } else {
                    TX_CHECK(tx_status);
                }

                assert(event_info.port == PORT_HOST);

                /* Invalid timestamp, error has occured */
                if ((event_info.time.nanosecond == 0) &&
                    (event_info.time.second_low == 0) &&
                    (event_info.time.second_high == 0)) {
                    break;
                }

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
                VAL_COVER(PTP_CLOCK, MAC_SYNC_FAILED);
                LOG_WARNING("PTP: MAC Sync failed, missing timestamps (received 0x%01x)", timestamps_received);
                tx_thread_sleep_ms(100); /* Wait before trying again so packets can settle */
                continue;
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

            /* Check there wasn't a reset while calculating the offset (timestamps could be corrupted) */
            tx_status = tx_event_flags_get(&ptp_mac_sync_events_group, PTP_CLOCK_EVENT_RESET, TX_OR, &event_flags, TX_NO_WAIT);
            if ((tx_status != TX_SUCCESS) && (tx_status != TX_NO_EVENTS)) error_handler();
            if (event_flags & PTP_CLOCK_EVENT_RESET) {
                continue;
            }

            /* Adjust time */
            if ((offset.second_high == 0) && (offset.second_low == 0)) {

                /* Count as synced when within 5 clock cycles (100ns) */
                mac_synced = ABS(offset.nanosecond) < (PTP_COUNTER_INCREMENT * 10);

                /* Coarse adjustment */
                if (ABS(offset.nanosecond) > MS_TO_NS(PTP_MAC_SYNC_FINE_ADJUST_THRESHOLD)) {
                    ptp_mac_adjust_time_coarse(&offset);
                    ptp_pi_controller_clear(&mac_sync_controller, tx_time_get_ms());

                    VAL_COVER(PTP_CLOCK, MAC_SYNC_ADJUST_COARSE);
                }

                /* Fine adjustment */
                else {

                    assert(mac_sync_controller.initialised);

                    /* Compute the controller output */
                    controller_output = ptp_pi_controller_compute(&mac_sync_controller,
                                                                  -offset.nanosecond,
                                                                  tx_time_get_ms());
                    controller_output = CONSTRAIN(controller_output,
                                                  PTP_MAC_SYNC_CONTROLLER_MIN_RATE,
                                                  PTP_MAC_SYNC_CONTROLLER_MAX_RATE);

                    /* Convert to addend and apply */
                    addend_adjustment = ((int64_t) PTP_COUNTER_ADDEND_DEFAULT * controller_output) / 1000000000LL;
                    addend            = (uint32_t) ((int64_t) PTP_COUNTER_ADDEND_DEFAULT + addend_adjustment);
                    ptp_mac_set_addend(addend);

                    VAL_COVER(PTP_CLOCK, MAC_SYNC_ADJUST_FINE);
                }

#if PTP_MAC_SYNC_PRINT_OFFSET
                LOG_INFO("PTP: MAC Sync offset is %li ns", offset.nanosecond);
#endif
            }

            /* Set time */
            else {
                ptp_mac_set_time(&switch_rx_timestamp);
                ptp_pi_controller_clear(&mac_sync_controller, tx_time_get_ms());
                mac_synced = false;
            }
        }
    }
}


uint8_t ptp_tx_timestamp_filter_packet(const NX_PACKET *packet_ptr, NX_PTP_TIME *timestamp) {

    tx_status_t             status = TX_SUCCESS;
    bool                    filter = false;
    ptp_packet_event_info_t event_info;

    if ((dummy_sync_packet_ptr != NULL) && (packet_ptr == dummy_sync_packet_ptr)) {

        filter = true;

        event_info.event = PTP_CLOCK_EVENT_TX_MAC_TIMESTAMP;
        event_info.time  = *timestamp;
        event_info.port  = PORT_HOST;
        status           = tx_queue_send(&ptp_mac_sync_queue, &event_info, TX_NO_WAIT);
        TX_CHECK(status);
    }

    return filter;
}
