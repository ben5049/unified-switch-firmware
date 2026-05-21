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

    return NX_SUCCESS;
}


/* Event callback for NetX PTP client */
UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data) {

    ptp_event_info_t event_info = {
        .event = event,
        .data  = event_data,
        .port  = (phy_index_t) callback_data};

    return tx_queue_send(&ptp_event_queue_handle, &event_info, TX_NO_WAIT);
}
