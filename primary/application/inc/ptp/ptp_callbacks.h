/*
 * ptp_callbacks.h
 *
 *  Created on: Aug 11, 2025
 *      Author: bens1
 */

#ifndef INC_PTP_CALLBACKS_H_
#define INC_PTP_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdatomic.h"
#include "hal.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"

#include "ptp_thread.h"


typedef struct {
    atomic_uint_fast32_t tx_timestamps_missed; /* Due to an eror in HAL_ETH_TxPtpCallback() */
    atomic_uint_fast32_t sync;
    atomic_uint_fast32_t new_master;
    atomic_uint_fast32_t master_timeout;
    atomic_uint_fast32_t clock_set;
    atomic_uint_fast32_t timestamps_extracted;
    atomic_uint_fast32_t clock_get;
    atomic_uint_fast32_t clock_adjusted;
    atomic_uint_fast32_t timestamps_sent;
} ptp_event_counters_t;


extern ptp_event_counters_t ptp_event_counters;


UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation, NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr, VOID *callback_data);
void HAL_ETH_TxPtpCallback(uint32_t *buff, ETH_TimeStampTypeDef *timestamp);
UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_CALLBACKS_H_ */
