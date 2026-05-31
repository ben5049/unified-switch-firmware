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
#include "ptp_sync.h"
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


void ptp_mac_sync_timer_callback(ULONG id) {
    tx_status_t status = tx_event_flags_set(&ptp_clock_events_handle, PTP_CLOCK_EVENT_MAC_SYNC, TX_OR);
    TX_CHECK(status);
}


void ptp_switch_sync_timer_callback(ULONG id) {
    tx_status_t status = tx_event_flags_set(&ptp_clock_events_handle, PTP_CLOCK_EVENT_SWITCH_SYNC, TX_OR);
    TX_CHECK(status);
}


UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation,
                        NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr,
                        VOID *callback_data) {

    tx_status_t      tx_status     = TX_SUCCESS;
    nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    UNUSED(tx_status); // TODO: use or remove

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

            if ((port_index_t) callback_data == ptp_port_connected_to_master) {

                /* Set the switch times */
                switch_status = switch_set_time_all(time_ptr);
                SWITCH_CHECK(switch_status);

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
            nx_status = NX_PTP_PARAM_ERROR;
            break;
    }

    return nx_status;
}


/* This thread synchronises the SJA1105 clocks with each other (if there are
 * multiple) and the STM32's MAC timestamp clock with the main SJA1105 clock. */
void ptp_clock_thread_entry(uint32_t initial_input) {

    nx_status_t nx_status = NX_SUCCESS;
    tx_status_t tx_status = TX_SUCCESS;

    ULONG events;

    /* Start the timers */
#if PTP_ENABLE_MAC_SYNC
    tx_status = tx_timer_activate(&ptp_mac_sync_timer);
    TX_CHECK(tx_status);
#endif
#if PTP_ENABLE_SWITCH_SYNC && (NUM_SWITCHES > 1)
    tx_status = tx_timer_activate(&ptp_switch_sync_timer);
    TX_CHECK(tx_status);
#endif

    while (1) {

        /* Wait for an event from the timers */
        tx_status = tx_event_flags_get(
            &ptp_clock_events_handle,
            PTP_CLOCK_EVENT_MAC_SYNC | PTP_CLOCK_EVENT_SWITCH_SYNC,
            TX_OR_CLEAR,
            &events,
            TX_WAIT_FOREVER);
        TX_CHECK(tx_status);

#if PTP_ENABLE_MAC_SYNC

        /* Syncronise the timestamps between the STM32's MAC and switch 0 */
        if (events & PTP_CLOCK_EVENT_MAC_SYNC) {
            nx_status = ptp_mac_sync();
            NX_CHECK(nx_status);
        }

#endif

#if PTP_ENABLE_SWITCH_SYNC

        /* Synchronise the timestamps between switches */
        if (events & PTP_CLOCK_EVENT_SWITCH_SYNC) {
            nx_status = ptp_switch_sync();
            NX_CHECK(nx_status);
        }

#endif
    }
}
