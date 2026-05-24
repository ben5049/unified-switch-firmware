/*
 * ptp_callbacks.c
 *
 *  Created on: May 16, 2026
 *      Author: bens1
 */

#include "ptp_thread.h"


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


/* Event callback for NetX PTP client */
UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data) {

    ptp_event_info_t event_info = {
        .event = event,
        .data  = event_data,
        .port  = (phy_index_t) callback_data};

    return tx_queue_send(&ptp_event_queue_handle, &event_info, TX_NO_WAIT);
}
