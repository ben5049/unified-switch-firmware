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
#include "ptp_callbacks.h"


#define COPY_NX_TIMESTAMP(into, from)            \
    do {                                         \
        (into).nanosecond  = (from).nanosecond;  \
        (into).second_low  = (from).second_low;  \
        (into).second_high = (from).second_high; \
    } while (0)


typedef struct {
    NX_PACKET  *packet_ptr;
    NX_PTP_TIME timestamp;
} nx_ptp_tx_info_t;


extern SHORT ptp_utc_offset;

extern NX_PTP_CLIENT ptp_client;

extern TX_THREAD ptp_thread_handle;
extern uint8_t   ptp_thread_stack[PTP_THREAD_STACK_SIZE];

extern TX_QUEUE ptp_tx_queue_handle;
extern uint8_t  ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * sizeof(nx_ptp_tx_info_t)];

void ptp_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_THREAD_H_ */
