/*
 * ptp_clock_callback.c
 *
 *  Created on: May 26, 2026
 *      Author: bens1
 */

#include "stdint.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "ptp.h"
#include "switch.h"
#include "utils.h"


TX_THREAD ptp_clock_thread_handle;
uint8_t   ptp_clock_thread_stack[PTP_CLOCK_THREAD_STACK_SIZE];

TX_QUEUE ptp_clock_queue;
uint32_t ptp_clock_queue_stack[PTP_CLOCK_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];


UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation,
                        NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr,
                        VOID *callback_data) {

    tx_status_t             tx_status     = TX_SUCCESS;
    nx_status_t             nx_status     = NX_SUCCESS;
    sja1105_status_t        switch_status = SJA1105_OK;
    port_index_t            port          = (port_index_t) callback_data;
    ptp_packet_event_info_t event_info;

    assert(port < NUM_PHYS);
    assert(client_ptr == &ptp_client[port]);

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

            if (port == ptp_port_connected_to_master) {

                ptp_event_counters.clock_set++;

                /* Set the switch times */
                switch_status = switch_set_time_all(time_ptr);
                SWITCH_CHECK(switch_status);
            }

            break;

        /* Extract timestamp from packet */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_EXTRACT:

            ptp_event_counters.timestamps_extracted++;

            /* Return timestamp stored at the beginning of the packet.  */
            ptp_packet_extract_timestamp(packet_ptr, time_ptr);
            break;

        /* Get clock */
        case NX_PTP_CLIENT_CLOCK_GET:

            ptp_event_counters.clock_get++;

            /* Use the local PTP clock which the application synchronises to SWITCH0 */
            ptp_mac_get_time(time_ptr);
            break;

        /* Adjust clock */
        case NX_PTP_CLIENT_CLOCK_ADJUST:

            if (port == ptp_port_connected_to_master) {

                ptp_event_counters.clock_adjusted++;

                event_info.event = PTP_CLOCK_EVENT_ADJUST;
                event_info.time  = *time_ptr;
                event_info.port  = port;
                tx_status        = tx_queue_send(&ptp_clock_queue, &event_info, TX_NO_WAIT);
                if ((tx_status != TX_SUCCESS) && (tx_status != TX_QUEUE_FULL)) error_handler();
            }

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
