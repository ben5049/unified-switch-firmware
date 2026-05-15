/*
 * ptp_thread.h
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#ifndef INC_PTP_THREAD_H_
#define INC_PTP_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "tx_api.h"
#include "nx_api.h"

#include "nx_app.h"


#define COPY_NX_TIMESTAMP(into, from)            \
    do {                                         \
        (into).nanosecond  = (from).nanosecond;  \
        (into).second_low  = (from).second_low;  \
        (into).second_high = (from).second_high; \
    } while (0)

#define PTP_MSG_SIZE_WORDS (sizeof(ptp_event_t) / sizeof(uint32_t))


typedef struct {
    uint32_t event;
    void    *event_data;
} ptp_event_t;

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


extern SHORT ptp_utc_offset;

extern NX_PTP_CLIENT ptp_client;

extern TX_THREAD ptp_thread_handle;
extern uint8_t   ptp_thread_stack[PTP_THREAD_STACK_SIZE];

extern TX_QUEUE ptp_tx_queue_handle;
extern uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

extern ptp_event_counters_t ptp_event_counters;


void ptp_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_THREAD_H_ */
