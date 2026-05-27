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

#include "tx_app.h"
#include "nx_app.h"
#include "phy_thread.h"


#define PTP_MSG_SIZE_WORDS ((sizeof(ptp_event_info_t) + 3) / 4) /* Round up */


typedef enum {

    /* PTP Client callback */
    PTP_CLIENT_EVENT_MASTER  = NX_PTP_CLIENT_EVENT_MASTER,
    PTP_CLIENT_EVENT_SYNC    = NX_PTP_CLIENT_EVENT_SYNC,
    PTP_CLIENT_EVENT_TIMEOUT = NX_PTP_CLIENT_EVENT_TIMEOUT,

    /* PTP TX Queue event */
    PTP_TX_EVENT_SEND_PACKET,

    /* PTP TX Event flags */
    PTP_TX_EVENT_MGMT_FREE   = 1UL << 8, /* All management routes have been used */
    PTP_TX_EVENT_PACKET_FREE = 1UL << 9, /* Ethernet driver is done with packet and it can be freed */

    /* PTP RX Queue event */
    PTP_RX_EVENT_RECEIVE_PACKET,

    /* PTP Clock queue event */
    PTP_CLOCK_EVENT_TX_MAC_TIMESTAMP,
    PTP_CLOCK_EVENT_RX_SWITCH_TIMESTAMP,
    PTP_CLOCK_EVENT_TX_SWITCH_TIMESTAMP,
    PTP_CLOCK_EVENT_RX_MAC_TIMESTAMP,

    /* PTP Clock event flags */
    PTP_CLOCK_EVENT_MAC_SYNC    = 1UL << 10, /* Time to sync the STM32's MAC clock with the main SJA1105's */
    PTP_CLOCK_EVENT_SWITCH_SYNC = 1UL << 11, /* Time to sync the SJA1105s' clocks together */

    PTP_EVENT_ALL = 0xffffffffUL

} ptp_event_t;

typedef struct {
    ptp_event_t event;
    union {
        void                 *data;
        NX_PACKET            *packet_ptr;
        NX_PTP_CLIENT_MASTER *master;
        NX_PTP_CLIENT_SYNC   *sync;
        NX_PTP_TIME           time;
    };
    port_index_t port;
} ptp_event_info_t;

typedef struct {

    /* TX */
    atomic_uint_fast32_t tx_packets_sent[NUM_PORTS];
    atomic_uint_fast32_t tx_packets_dropped[NUM_PORTS];
    atomic_uint_fast32_t tx_timestamps_received[NUM_PORTS];
    atomic_uint_fast32_t tx_timestamps_missed[NUM_PORTS];

    /* RX */
    atomic_uint_fast32_t rx_no_meta;
    atomic_uint_fast32_t rx_invalid_vlan;
    atomic_uint_fast32_t rx_meta_dropped;
    atomic_uint_fast32_t rx_packets_dropped;
    atomic_uint_fast32_t rx_client_not_found[NUM_PORTS];

    /* Events */
    atomic_uint_fast32_t sync;
    atomic_uint_fast32_t new_master;
    atomic_uint_fast32_t master_timeout;

    // TODO: Use?
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
extern uint8_t   ptp_tx_thread_stack[PTP_TX_THREAD_STACK_SIZE];
extern TX_THREAD ptp_rx_thread_handle;
extern uint8_t   ptp_rx_thread_stack[PTP_RX_THREAD_STACK_SIZE];
extern TX_THREAD ptp_clock_thread_handle;
extern uint8_t   ptp_clock_thread_stack[PTP_CLOCK_THREAD_STACK_SIZE];

extern TX_QUEUE ptp_event_queue_handle;
extern uint32_t ptp_event_queue_stack[PTP_EVENT_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_tx_queue_handle;
extern uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_rx_packet_queue_handle;
extern uint32_t ptp_rx_packet_queue_stack[PTP_RX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_rx_meta_queue_handle;
extern uint32_t ptp_rx_meta_queue_stack[PTP_RX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_clock_queue_handle;
extern uint32_t ptp_clock_queue_stack[PTP_CLOCK_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

extern TX_EVENT_FLAGS_GROUP ptp_tx_events_handle;
extern TX_EVENT_FLAGS_GROUP ptp_clock_events_handle;

extern TX_TIMER ptp_mac_sync_timer;
extern TX_TIMER ptp_switch_sync_timer;

extern NX_PTP_CLIENT        ptp_client[NUM_PHYS];
extern SHORT                ptp_utc_offset;
extern ptp_event_counters_t ptp_event_counters;


/* Thread functions */
void ptp_event_thread_entry(uint32_t initial_input);
void ptp_tx_thread_entry(uint32_t initial_input);
void ptp_rx_thread_entry(uint32_t initial_input);
void ptp_clock_thread_entry(uint32_t initial_input);

/* Timer callbacks */
void ptp_mac_sync_timer_callback(ULONG id);
void ptp_switch_sync_timer_callback(ULONG id);

/* Filtering functions to place in Ethernet driver to intercept packets */
uint8_t ptp_tx_filter_packet_send(NX_PACKET *packet_ptr);
uint8_t ptp_tx_filter_packet_free(NX_PACKET *packet_ptr);
uint8_t ptp_rx_filter_packet(NX_PACKET *packet_ptr, uint32_t ts[2]);
uint8_t ptp_clock_tx_filter_packet(NX_PACKET *packet_ptr, NX_PTP_TIME *timestamp);

/* Callbacks for PTP client */
UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation, NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr, VOID *callback_data);
UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_THREAD_H_ */
