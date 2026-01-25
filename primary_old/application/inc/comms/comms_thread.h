/*
 * comms.h
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#ifndef INC_COMMS_THREAD_H_
#define INC_COMMS_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdatomic.h"
#include "tx_api.h"
#include "zenoh-pico.h"
#include "main.h"

#include "config.h"
#include "tx_app.h"
#include "state_machine.h"


typedef struct {
    atomic_uint_fast32_t closures;
    atomic_uint_fast32_t restarts;
    atomic_uint_fast32_t reconnection_attempts;
    atomic_uint_fast32_t connections;
    atomic_uint_fast32_t heartbeats_missed;
    atomic_uint_fast32_t packets_sent;
    uint64_t             bytes_sent;     /* No 64-bit atomics so accesses should use TX_DISABLE */
    atomic_uint_fast32_t packets_received;
    uint64_t             bytes_received; /* No 64-bit atomics so accesses should use TX_DISABLE */
} zenoh_event_counters_t;


/* Exported variables */
extern uint8_t      comms_thread_stack[COMMS_THREAD_STACK_SIZE];
extern TX_THREAD    comms_thread_handle;
extern uint8_t      zenoh_byte_pool_buffer[ZENOH_MEM_POOL_SIZE];
extern TX_BYTE_POOL zenoh_byte_pool;

extern zenoh_event_counters_t zenoh_events;

extern z_owned_publisher_t stats_pub;


tx_status_t zenoh_connected(bool update_state_machine);
tx_status_t zenoh_disconnected(bool update_state_machine);

void comms_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_COMMS_THREAD_H_ */
