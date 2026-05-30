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
#include "utils.h"
#include "ptp_thread.h"
#include "ptp_utils.h"
#include "switch_thread.h"
#include "switch_utils.h"


TX_THREAD ptp_clock_thread_handle;
uint8_t   ptp_clock_thread_stack[PTP_CLOCK_THREAD_STACK_SIZE];

TX_QUEUE ptp_clock_queue_handle;
uint32_t ptp_clock_queue_stack[PTP_CLOCK_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];

TX_EVENT_FLAGS_GROUP ptp_clock_events_handle;

TX_TIMER ptp_mac_sync_timer;
TX_TIMER ptp_switch_sync_timer;

static volatile NX_PACKET *dummy_packet_ptr = NULL; /* Packet pointer of dummy packet */


void ptp_mac_sync_timer_callback(ULONG id) {
    if (tx_event_flags_set(&ptp_clock_events_handle, PTP_CLOCK_EVENT_MAC_SYNC, TX_OR) != TX_SUCCESS) error_handler();
}


void ptp_switch_sync_timer_callback(ULONG id) {
    if (tx_event_flags_set(&ptp_clock_events_handle, PTP_CLOCK_EVENT_SWITCH_SYNC, TX_OR) != TX_SUCCESS) error_handler();
}


UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation,
                        NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr,
                        VOID *callback_data) {

    tx_status_t tx_status = TX_SUCCESS;
    nx_status_t status    = NX_SUCCESS;

    switch (operation) {

        /* No initialisation to do */
        case NX_PTP_CLIENT_CLOCK_INIT:
            break;

        /* Set all switch clocks and stm32 clock if this client is connected to
         * the grandmaster
         *
         * Note: This should be done as little as possible since it can corrupt
         *       time stamps.
         */
        case NX_PTP_CLIENT_CLOCK_SET:

            if ((port_index_t) callback_data == port_connected_to_master) {

                /* Set the switch times */
                if (switch_set_time_all(time_ptr) != SJA1105_OK) error_handler();

                /* Set the STM32's MAC time */
                ptp_mac_set_time(time_ptr);
            }

            break;

        /* Extract timestamp from packet */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_EXTRACT:

            /* Return timestamp stored at the beginning of the packet.  */
            ptp_packet_extract_timestamp(packet_ptr, time_ptr);
            break;

        /* Get clock */
        case NX_PTP_CLIENT_CLOCK_GET:

            /* Use the local PTP clock which the application synchronises to SWITCH0 */
            ptp_mac_get_time(time_ptr);
            break;

        /* Adjust clock */
        case NX_PTP_CLIENT_CLOCK_ADJUST:
            // TODO: check if this client is connected to the grandmaster and discipline SWITCH0 if it is
            break;

        /* Prepare timestamp for current packet. Not used because the timestamp
         * needs to be immediately fetched from the SJA1105. This is handled by
         * the application */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_PREPARE:
            break;

        /* Update soft timer. Not used by hardware callback function */
        case NX_PTP_CLIENT_CLOCK_SOFT_TIMER_UPDATE:
            break;

        default:
            status = NX_PTP_PARAM_ERROR;
            break;
    }

    return status;
}


/* This thread synchronises the SJA1105 clocks with each other (if there are
 * multiple) and the STM32's MAC timestamp clock with the main SJA1105 clock. */
void ptp_clock_thread_entry(uint32_t initial_input) {

    nx_status_t nx_status = NX_SUCCESS;
    tx_status_t tx_status = TX_SUCCESS;

    ULONG                   events;
    ptp_packet_event_info_t event_info;

    NX_PACKET *packet_ptr;

    NX_PTP_TIME mac_tx_timestamp;    /* t1 */
    NX_PTP_TIME switch_rx_timestamp; /* t2 */
    NX_PTP_TIME switch_tx_timestamp; /* t3 */
    NX_PTP_TIME mac_rx_timestamp;    /* t4 */

    NX_PTP_TIME switch_tx_correction;
    NX_PTP_TIME offset;

    uint8_t  timestamps_received;
    uint16_t dummy_packet_length;
    uint16_t host_speed_mbps;

    uint32_t new_ptp_clock_rate;
    uint32_t ptp_clock_rates[NUM_SWITCHES] = {
        [SWITCH0] = SJA1105_PTP_CLK_RATE_DEFAULT,
#if NUM_SWITCHES > 1
        [SWITCH1] = SJA1105_PTP_CLK_RATE_DEFAULT,
#endif
    };
    uint8_t skip_switch_sync[NUM_SWITCHES] = {0};


    /* Start the timers */
    tx_status = tx_timer_activate(&ptp_mac_sync_timer);
    if (tx_status != TX_SUCCESS) error_handler();
#if NUM_SWITCHES > 1
    tx_status = tx_timer_activate(&ptp_switch_sync_timer);
    if (tx_status != TX_SUCCESS) error_handler();
#endif

    while (1) {

        /* Wait for an event from the timers */
        tx_status = tx_event_flags_get(
            &ptp_clock_events_handle,
            PTP_CLOCK_EVENT_MAC_SYNC | PTP_CLOCK_EVENT_SWITCH_SYNC,
            TX_OR_CLEAR,
            &events,
            MAX(PTP_MAC_SYNC_INTERVAL, PTP_SWITCH_SYNC_INTERVAL) * 5 /* Break out of deadlocks */
        );
        if (tx_status != TX_SUCCESS) error_handler();

        /* Syncronise the timestamps between the STM32's MAC and switch 0 */
        if (events & PTP_CLOCK_EVENT_MAC_SYNC) {

            /* Create a dummy sync packet that will look convincing enough to make
             * the STM32's MAC timestamp it */
            nx_status = ptp_create_dummy_sync(&packet_ptr);
            if (nx_status != NX_SUCCESS) {
#if DEBUG
                error_handler();
#endif
                continue;
            };

            /* Save packet pointer for callback filtering and length for timestamp corrections */
            dummy_packet_ptr    = packet_ptr;
            dummy_packet_length = packet_ptr->nx_packet_length;

            /* Send the packet so that it bounces back into the host port */
            event_info.event      = PTP_TX_EVENT_SEND_PACKET;
            event_info.packet_ptr = packet_ptr;
            event_info.port       = PORT_HOST;
            tx_status             = tx_queue_send(&ptp_tx_queue_handle, &event_info, TX_NO_WAIT);
            if (tx_status != TX_SUCCESS) {
                nx_status = nx_packet_release(packet_ptr);
                if (nx_status != NX_SUCCESS) error_handler();
#if DEBUG
                error_handler();
#endif
                continue;
            }

            /* Gather up all the timestamps */
            timestamps_received = 0;
            while (timestamps_received < PTP_CLOCK_NUM_TIMESTAMPS) {

                tx_status = tx_queue_receive(&ptp_clock_queue_handle, &event_info, MS_TO_TICKS(PTP_CLOCK_TIMEOUT));
                if (tx_status != TX_SUCCESS) error_handler(); // TODO: my does disconnecting a device cause this to fail???

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
            dummy_packet_ptr = NULL;

            /* The switch's egress timestamp in this scenario is for the start of
             * the META frame so we must subtract the time it took to send the PTP
             * packet and the time between frames (AH1704 p65).
             *
             * Note: An Ethernet packet cannot be shorter than 64 bytes.
             */
            if (switch_get_speed(PORT_HOST, &host_speed_mbps) != SJA1105_OK) error_handler();
            switch_tx_correction.nanosecond  = (MAX(dummy_packet_length, 64) + SJA1105_IFG + SJA1105_PREAMBLE) * 8 * MHZ_TO_NS(host_speed_mbps);
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
        }

        /* Synchronise the timestamps between switches */
        if (events & PTP_CLOCK_EVENT_SWITCH_SYNC) {

#if NUM_SWITCHES > 1

            /* This is agnostic to the number of switches in the system */
            for (switch_index_t i = SWITCH1; i < NUM_SWITCHES; i++) {

                /* After reaching equilibrium skip PTP_SWITCH_SYNC_SKIP syncs since they aren't necessary */
                if (skip_switch_sync[i]) {
                    skip_switch_sync[i]--;
                    continue;
                }

                /* Get the offset between the two switches */
                if (switch_get_timestamp_offsets(SWITCH0, i, &offset) != SJA1105_OK) error_handler();

                assert(offset.second_high == 0);
                assert(offset.second_low == 0);

                /* Adjust the PTP clock rate to sync slave with master within +-3
                 * ticks (+-24ns)
                 *
                 * Note: The switches share a common oscillator so they tick at
                 *       the same rate, the only thing that can cause offsets is
                 *       jitter when the application adjusts their rates.
                 */
                if (offset.nanosecond > (SJA1105_NS_PER_TS_TICK * 3)) {
                    new_ptp_clock_rate = SJA1105_PTP_CLK_RATE_SLIGHTLY_FASTER;
                } else if (offset.nanosecond < -(SJA1105_NS_PER_TS_TICK * 3)) {
                    new_ptp_clock_rate = SJA1105_PTP_CLK_RATE_SLIGHTLY_SLOWER;
                } else {
                    new_ptp_clock_rate  = SJA1105_PTP_CLK_RATE_DEFAULT;
                    skip_switch_sync[i] = PTP_SWITCH_SYNC_SKIP;
                }

                /* Write the new rate */
                if (new_ptp_clock_rate != ptp_clock_rates[i]) {
                    ptp_clock_rates[i] = new_ptp_clock_rate;
                    if (SJA1105_SetPTPClockRate(&switch_handles[i], ptp_clock_rates[i]) != SJA1105_OK) error_handler();
                    LOG_INFO("Switch %d current offset = %li ns", i, offset.nanosecond);
                }
            }

#else
            error_handler();
#endif
        }
    }
}


uint8_t ptp_clock_tx_filter_packet(NX_PACKET *packet_ptr, NX_PTP_TIME *timestamp) {

    bool                    filter = false;
    ptp_packet_event_info_t event_info;

    if ((dummy_packet_ptr != NULL) && (packet_ptr == dummy_packet_ptr)) {

        filter = true;

        event_info.event = PTP_CLOCK_EVENT_TX_MAC_TIMESTAMP;
        event_info.time  = *timestamp;
        event_info.port  = PORT_HOST;
        if (tx_queue_send(&ptp_clock_queue_handle, &event_info, TX_NO_WAIT) != TX_SUCCESS) error_handler();
    }

    return filter;
}
