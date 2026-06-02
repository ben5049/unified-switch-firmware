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
#include "utils.h"
#include "ptp_thread.h"
#include "ptp_utils.h"
#include "switch_thread.h"
#include "switch_utils.h"


UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation,
                        NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr,
                        VOID *callback_data) {

    tx_status_t      tx_status     = TX_SUCCESS;
    nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;
    port_index_t     port          = (port_index_t) callback_data;

    assert(port < NUM_PHYS);
    assert(client_ptr == &ptp_client[port]);

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

            if (port == ptp_port_connected_to_master) {

                ptp_event_counters.clock_set++;

                /* Set the switch times */
                switch_status = switch_set_time_all(time_ptr);
                SWITCH_CHECK(switch_status);

                /* Set the STM32's MAC time */
                // ptp_mac_set_time(time_ptr); TODO: something
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

                // TODO: check if this client is connected to the grandmaster and discipline SWITCH0 if it is

                // TODO: Use fine
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
