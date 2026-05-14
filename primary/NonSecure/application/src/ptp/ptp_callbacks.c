/*
 * ptp_callbacks.c
 *
 *  Created on: Aug 11, 2025
 *      Author: bens1
 */

#include "math.h"
#include "stdint.h"
#include "hal.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"

#include "nx_app.h"
#include "ptp_callbacks.h"
#include "utils.h"
#include "config.h"


#define NX_PTP_NANOSECONDS_PER_SEC 1000000000L




/* Clock callback for NetX PTP client */

UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data) {
    NX_PARAMETER_NOT_USED(callback_data);

    ptp_event_t ptp_event = {.event = event, .event_data = event_data};

    return tx_queue_send(&ptp_tx_queue_handle, &ptp_event, TX_NO_WAIT);
}
