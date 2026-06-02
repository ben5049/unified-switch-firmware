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


#define PTP_CLIENT_MSG_SIZE_WORDS ((sizeof(ptp_client_event_info_t) + 3) / 4) /* Round up */
#define PTP_PACKET_MSG_SIZE_WORDS ((sizeof(ptp_packet_event_info_t) + 3) / 4) /* Round up */


typedef enum {
    PTP_MESSAGE_TYPE_SYNC                  = 0x0, /* RX Event message */
    PTP_MESSAGE_TYPE_DELAY_REQ             = 0x1,
    PTP_MESSAGE_TYPE_PDELAY_REQ            = 0x2, /* RX Event message */
    PTP_MESSAGE_TYPE_PDELAY_RESP           = 0x3, /* RX Event message */
    PTP_MESSAGE_TYPE_FOLLOW_UP             = 0x8,
    PTP_MESSAGE_TYPE_DELAY_RESP            = 0x9,
    PTP_MESSAGE_TYPE_PDELAY_RESP_FOLLOW_UP = 0xa,
    PTP_MESSAGE_TYPE_ANNOUNCE              = 0xb,
    PTP_MESSAGE_TYPE_SIGNALLING            = 0xc,
    PTP_MESSAGE_TYPE_MANAGEMENT            = 0xd,

    PTP_MESSAGE_TYPE_MASK = 0x0f,
} ptp_message_type_t;


typedef enum {

    /* PTP Client callback */
    PTP_CLIENT_EVENT_MASTER  = NX_PTP_CLIENT_EVENT_MASTER,
    PTP_CLIENT_EVENT_SYNC    = NX_PTP_CLIENT_EVENT_SYNC,
    PTP_CLIENT_EVENT_TIMEOUT = NX_PTP_CLIENT_EVENT_TIMEOUT,

    /* PTP Event flags */
    PTP_EVENT_CLIENT     = 1UL << 6, /* Master, SYNC or timeout */
    PTP_EVENT_PRINT_TIME = 1UL << 7,

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
        struct {
            NX_PTP_CLIENT_MASTER master;
            uint8_t              _grandmaster_identity[NX_PTP_CLOCK_PORT_IDENTITY_SIZE];
        };
        NX_PTP_CLIENT_SYNC sync;
    };
    port_index_t port;
} ptp_client_event_info_t;


typedef struct {
    ptp_event_t event;
    union {
        NX_PACKET  *packet_ptr;
        NX_PTP_TIME time;
    };
    port_index_t port;
} ptp_packet_event_info_t;

typedef struct {

    /* TX */
    atomic_uint_fast32_t tx_packets_sent[NUM_PORTS];
    atomic_uint_fast32_t tx_packets_dropped[NUM_PORTS];
    atomic_uint_fast32_t tx_packets_invalid_port;
    atomic_uint_fast32_t tx_timestamps_received[NUM_PORTS];
    atomic_uint_fast32_t tx_timestamps_missed[NUM_PORTS];
    atomic_uint_fast32_t tx_slow; /* Took multiple polls for the management route to free */

    /* RX */
    atomic_uint_fast32_t rx_meta;       /* Number of META frames filtered */
    atomic_uint_fast32_t rx_packets;    /* Number of PTP packets filtered */
    atomic_uint_fast32_t rx_events;     /* Number of event PTP packets received (SYNC, PDELAY_REQ, PDELAY_RESP) */
    atomic_uint_fast32_t rx_general;    /* Number of general PTP packets received */
    atomic_uint_fast32_t rx_no_meta;    /* Expected a META frame but didn't get one */
    atomic_uint_fast32_t rx_wrong_dst;  /* gPTP Ethertype but wrong destination address */
    atomic_uint_fast32_t rx_own_packet; /* gPTP Packet send and received by us */
    atomic_uint_fast32_t rx_invalid_vlan;
    atomic_uint_fast32_t rx_meta_dropped;
    atomic_uint_fast32_t rx_packets_dropped;
    atomic_uint_fast32_t rx_client_not_found[NUM_PORTS];

    /* Events */
    atomic_uint_fast32_t sync;
    atomic_uint_fast32_t new_master;
    atomic_uint_fast32_t master_timeout;

    /* Clock events */
    atomic_uint_fast32_t clock_set;
    atomic_uint_fast32_t timestamps_extracted;
    atomic_uint_fast32_t clock_get;
    atomic_uint_fast32_t clock_adjusted;
    atomic_uint_fast32_t mac_sync_failed;
} ptp_event_counters_t;


/* Exported variables */

extern TX_THREAD ptp_event_thread_handle;
extern uint8_t   ptp_event_thread_stack[PTP_EVENT_THREAD_STACK_SIZE];
extern TX_THREAD ptp_tx_thread_handle;
extern uint8_t   ptp_tx_thread_stack[PTP_TX_THREAD_STACK_SIZE];
extern TX_THREAD ptp_rx_thread_handle;
extern uint8_t   ptp_rx_thread_stack[PTP_RX_THREAD_STACK_SIZE];
extern TX_THREAD ptp_mac_sync_thread_handle;
extern uint8_t   ptp_mac_sync_thread_stack[PTP_MAC_SYNC_THREAD_STACK_SIZE];
#if NUM_SWITCHES > 1
extern TX_THREAD ptp_switch_sync_thread_handle;
extern uint8_t   ptp_switch_sync_thread_stack[PTP_MAC_SYNC_THREAD_STACK_SIZE];
#endif

extern TX_QUEUE ptp_event_queue_handle;
extern uint32_t ptp_event_queue_stack[PTP_EVENT_QUEUE_SIZE * PTP_CLIENT_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_tx_queue_handle;
extern uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_rx_packet_queue_handle;
extern uint32_t ptp_rx_packet_queue_stack[PTP_RX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_rx_meta_queue_handle;
extern uint32_t ptp_rx_meta_queue_stack[PTP_RX_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];
extern TX_QUEUE ptp_mac_sync_queue_handle;
extern uint32_t ptp_mac_sync_queue_stack[PTP_MAC_SYNC_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];

extern TX_EVENT_FLAGS_GROUP ptp_events_handle;
extern TX_EVENT_FLAGS_GROUP ptp_tx_events_handle;
extern TX_EVENT_FLAGS_GROUP ptp_mac_sync_events_handle;
#if NUM_SWITCHES > 1
extern TX_EVENT_FLAGS_GROUP ptp_switch_sync_events_handle;
#endif

#if PTP_PRINT_TIME_INTERVAL
extern TX_TIMER ptp_events_print_time_timer;
#endif
extern TX_TIMER ptp_mac_sync_timer;
#if NUM_SWITCHES > 1
extern TX_TIMER ptp_switch_sync_timer;
#endif

extern NX_PTP_CLIENT         ptp_client[NUM_PHYS];
extern SHORT                 ptp_utc_offset;
extern ptp_event_counters_t  ptp_event_counters;
extern volatile port_index_t ptp_port_connected_to_master;


/* Thread functions */
void ptp_event_thread_entry(uint32_t initial_input);
void ptp_tx_thread_entry(uint32_t initial_input);
void ptp_rx_thread_entry(uint32_t initial_input);
void ptp_clock_thread_entry(uint32_t initial_input);
void ptp_mac_sync_thread_entry(uint32_t initial_input);
#if NUM_SWITCHES > 1
void ptp_switch_sync_thread_entry(uint32_t initial_input);
#endif

/* Timer callbacks */
#if PTP_PRINT_TIME_INTERVAL
void ptp_events_print_time_timer_callback(ULONG id);
#endif
void ptp_mac_sync_timer_callback(ULONG id);
#if NUM_SWITCHES > 1
void ptp_switch_sync_timer_callback(ULONG id);
#endif

/* Filtering functions to place in Ethernet driver to intercept packets */
uint8_t ptp_tx_filter_packet_send(NX_PACKET *packet_ptr);
uint8_t ptp_tx_filter_packet_free(NX_PACKET *packet_ptr);
uint8_t ptp_rx_filter_packet(NX_PACKET *packet_ptr, uint32_t ts[2]);
uint8_t ptp_tx_timestamp_filter_packet(NX_PACKET *packet_ptr, NX_PTP_TIME *timestamp);

/* Callbacks for PTP client */
UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation, NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr, VOID *callback_data);
UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_THREAD_H_ */
