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
uint32_t ptp_clock_queue_stack[PTP_CLOCK_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

static volatile NX_PACKET *dummy_packet_ptr = NULL; /* Packet pointer of dummy packet */


UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation,
                        NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr,
                        VOID *callback_data) {

    nx_status_t status = NX_SUCCESS;

    switch (operation) {

        /* No initialisation to do */
        case NX_PTP_CLIENT_CLOCK_INIT:
            break;

        /* Set clock */
        // TODO: Set all switch clocks and stm32 clock if this client is connected to the grandmaster
        case NX_PTP_CLIENT_CLOCK_SET:
            // TX_DISABLE

            // /* Get Start Tick*/
            // tickstart = HAL_GetTick();

            // /* Wait to get PTP control or timeout occurred */
            // while (((HAL_GetTick() - tickstart) > HAL_PTP_TIMEOUT)) {
            //     if (__HAL_ETH_GET_PTP_CONTROL(&eth_handle, ETH_MACTSCR_TSUPDT) == 0) {
            //         NX_PTP_Status = NX_WAIT_ERROR;
            //         break;
            //     }
            // }

            // time.Seconds     = time_ptr->second_low;
            // time.NanoSeconds = time_ptr->nanosecond;
            // HAL_ETH_PTP_SetTime(&eth_handle, &time);

            // TX_RESTORE
            break;

        /* Extract timestamp from packet */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_EXTRACT:

            /* Return timestamp stored at the beginning of the packet.  */
            ptp_packet_extract_timestamp(packet_ptr, time_ptr);
            break;

        /* Get clock. Use the local PTP clock which the application synchronises to SWITCH0 */
        /* TODO: God knows what this function does */
        case NX_PTP_CLIENT_CLOCK_GET:

            // TX_DISABLE

            // HAL_ETH_PTP_GetTime(&eth_handle, &time);
            // sec1 = time.Seconds;
            // ns   = time.NanoSeconds;
            // HAL_ETH_PTP_GetTime(&eth_handle, &time);
            // sec2                  = time.Seconds;
            // time_ptr->second_high = 0;

            // /* The offset standard deviation is below 50 ns */
            // time_ptr->second_low = ns < 500000000UL ? sec2 : sec1;
            // time_ptr->nanosecond = (LONG) ns;

            // TX_RESTORE
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
 * multiple) and the STM32 timestamp clock with the main SJA1105 clock. */
void ptp_clock_thread_entry(uint32_t initial_input) {

    nx_status_t nx_status = NX_SUCCESS;
    tx_status_t tx_status = TX_SUCCESS;

    NX_PACKET       *packet_ptr;
    ptp_event_info_t event_info;

    NX_PTP_TIME mac_tx_timestamp;    /* t1 */
    NX_PTP_TIME switch_rx_timestamp; /* t2 */
    NX_PTP_TIME switch_tx_timestamp; /* t3 */
    NX_PTP_TIME mac_rx_timestamp;    /* t4 */

    NX_PTP_TIME switch_tx_correction;
    NX_PTP_TIME offset;

    uint8_t  timestamps_received;
    uint16_t dummy_packet_length;
    uint16_t host_speed_mbps;

    while (1) {

        /* Create a dummy sync packet that will look convincing enough to make
         * the MAC timestamp it */
        nx_status = ptp_create_dummy_sync(&packet_ptr);
        if (nx_status != NX_SUCCESS) error_handler();

        /* Save packet pointer for callback filtering and length for timestamp corrections */
        dummy_packet_ptr    = packet_ptr;
        dummy_packet_length = packet_ptr->nx_packet_length;

        /* Send the packet so that it bounces back into the host port */
        event_info.event = PTP_TX_EVENT_SEND_PACKET;
        event_info.data  = packet_ptr;
        event_info.port  = PORT_HOST;
        tx_status        = tx_queue_send(&ptp_tx_queue_handle, &event_info, TX_NO_WAIT);
        if (tx_status != TX_SUCCESS) error_handler();

        /* Gather up all the timestamps */
        timestamps_received = 0;
        while (timestamps_received < PTP_CLOCK_NUM_TIMESTAMPS) {

            tx_status = tx_queue_receive(&ptp_clock_queue_handle, &event_info, PTP_CLOCK_TIMEOUT);
            if (tx_status != TX_SUCCESS) error_handler();

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

        tx_thread_sleep_ms(100);
    }
}


uint8_t ptp_clock_tx_filter_packet(NX_PACKET *packet_ptr, NX_PTP_TIME *timestamp) {

    bool             filter = false;
    ptp_event_info_t event_info;

    if ((dummy_packet_ptr != NULL) && (packet_ptr == dummy_packet_ptr)) {

        filter = true;

        event_info.event = PTP_CLOCK_EVENT_TX_MAC_TIMESTAMP;
        event_info.time  = *timestamp;
        event_info.port  = PORT_HOST;
        if (tx_queue_send(&ptp_clock_queue_handle, &event_info, TX_NO_WAIT) != TX_SUCCESS) error_handler();
    }

    return filter;
}
