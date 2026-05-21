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
#include "stdatomic.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"

#include "nx_app.h"
#include "phy_thread.h"


#define COPY_NX_TIMESTAMP(into, from)            \
    do {                                         \
        (into).nanosecond  = (from).nanosecond;  \
        (into).second_low  = (from).second_low;  \
        (into).second_high = (from).second_high; \
    } while (0)

#define PTP_MSG_SIZE_WORDS ((sizeof(ptp_event_t) + sizeof(uint32_t) - 1) / sizeof(uint32_t)) /* Round up */


typedef enum {

    /* PTP Client callback */
    PTP_CLIENT_EVENT_MASTER  = NX_PTP_CLIENT_EVENT_MASTER,
    PTP_CLIENT_EVENT_SYNC    = NX_PTP_CLIENT_EVENT_SYNC,
    PTP_CLIENT_EVENT_TIMEOUT = NX_PTP_CLIENT_EVENT_TIMEOUT,

    /* PTP TX Queue event */
    PTP_TX_EVENT_SEND_PACKET,

    /* PTP TX Event flags */
    PTP_TX_EVENT_MGMT_FREE   = 1 << 8, /* All management routes have been used */
    PTP_TX_EVENT_PACKET_FREE = 1 << 9, /* Ethernet driver is done with packet and it can be freed */

    /* PTP RX Queue event */
    PTP_RX_EVENT_RECEIVE_PACKET,

    PTP_TX_EVENT_ALL = 0xffffffff

} ptp_event_t;

typedef struct {
    ptp_event_t event;
    void       *data;
    phy_index_t port;
} ptp_event_info_t;

typedef struct {

    /* TX */
    atomic_uint_fast32_t tx_packets_sent[NUM_PHYS];
    atomic_uint_fast32_t tx_packets_dropped[NUM_PHYS];
    atomic_uint_fast32_t tx_timestamps_received[NUM_PHYS];
    atomic_uint_fast32_t tx_timestamps_missed[NUM_PHYS];

    /* Events */
    atomic_uint_fast32_t sync;
    atomic_uint_fast32_t new_master;
    atomic_uint_fast32_t master_timeout;

    // atomic_uint_fast32_t clock_set;
    // atomic_uint_fast32_t timestamps_extracted;
    // atomic_uint_fast32_t clock_get;
    // atomic_uint_fast32_t clock_adjusted;
    // atomic_uint_fast32_t timestamps_sent;
} ptp_event_counters_t;


/* Exported variables */

extern TX_THREAD ptp_event_thread_handle;
extern uint8_t   ptp_event_thread_stack[PTP_EVENT_THREAD_STACK_SIZE];
extern TX_THREAD ptp_tx_thread_handle;
extern uint8_t   ptp_tx_thread_stack[PTP_EVENT_THREAD_STACK_SIZE];
extern TX_THREAD ptp_rx_thread_handle;
extern uint8_t   ptp_rx_thread_stack[PTP_RX_THREAD_STACK_SIZE];

extern TX_QUEUE ptp_event_queue_handle;
extern uint32_t ptp_event_queue_stack[PTP_EVENT_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_tx_queue_handle;
extern uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_rx_packet_queue_handle;
extern uint32_t ptp_rx_packet_queue_stack[PTP_RX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_rx_meta_queue_handle;
extern uint32_t ptp_rx_meta_queue_stack[PTP_RX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

extern TX_EVENT_FLAGS_GROUP ptp_tx_events_handle;

extern NX_PTP_CLIENT        ptp_client[NUM_PHYS];
extern SHORT                ptp_utc_offset;
extern ptp_event_counters_t ptp_event_counters;


void ptp_event_thread_entry(uint32_t initial_input);
void ptp_tx_thread_entry(uint32_t initial_input);
void ptp_rx_thread_entry(uint32_t initial_input);

uint8_t ptp_tx_filter_packet_send(NX_PACKET *packet_ptr);
uint8_t ptp_tx_filter_packet_free(NX_PACKET *packet_ptr);

UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation, NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr, VOID *callback_data);
UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_THREAD_H_ */
