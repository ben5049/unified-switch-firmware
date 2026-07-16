/*
 * sequencer.h
 *
 *  Created on: Sep 3, 2025
 *      Author: bens1
 */

#ifndef INC_SEQUENCER_H_
#define INC_SEQUENCER_H_

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


extern TX_THREAD            sequencer_thread_handle;
extern uint8_t              sequencer_thread_stack[SEQUENCER_THREAD_STACK_SIZE];
extern TX_EVENT_FLAGS_GROUP sequencer_events_handle;


void sequencer_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_SEQUENCER_H_ */
