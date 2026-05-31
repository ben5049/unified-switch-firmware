/*
 * state_machine.h
 *
 *  Created on: Sep 3, 2025
 *      Author: bens1
 */

#ifndef INC_STATE_MACHINE_H_
#define INC_STATE_MACHINE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"

#include "config.h"
#include "tx_app.h"


typedef enum {
    STATE_MACHINE_NX_LINK_UP               = 1 << 0,
    STATE_MACHINE_NX_LINK_DOWN             = 1 << 1,
    STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED   = 1 << 2,
    STATE_MACHINE_NX_IP_ADDRESS_UNASSIGNED = 1 << 3,
    STATE_MACHINE_ZENOH_CONNECTED          = 1 << 4,
    STATE_MACHINE_ZENOH_DISCONNECTED       = 1 << 5,
} state_machine_event_t;


extern TX_THREAD            state_machine_thread_handle;
extern uint8_t              state_machine_thread_stack[STATE_MACHINE_THREAD_STACK_SIZE];
extern TX_EVENT_FLAGS_GROUP state_machine_events_handle;


void state_machine_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_STATE_MACHINE_H_ */
