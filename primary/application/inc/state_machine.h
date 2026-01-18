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
#include "tx_api.h"

#include "config.h"


#define STATE_MACHINE_NX_LINK_UP             ((ULONG) 1 << 0)
#define STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED ((ULONG) 1 << 1)
#define STATE_MACHINE_ZENOH_CONNECTED        ((ULONG) 1 << 2)
#define STATE_MACHINE_ZENOH_DISCONNECTED     ((ULONG) 1 << 3)
#define STATE_MACHINE_UPDATE                 ((ULONG) 1 << 31) /* Must be set for the state machine thread to react */


extern TX_THREAD            state_machine_thread_handle;
extern uint8_t              state_machine_thread_stack[STATE_MACHINE_THREAD_STACK_SIZE];
extern TX_EVENT_FLAGS_GROUP state_machine_events_handle;


void state_machine_thread_entry(uint32_t initial_input);

#ifdef __cplusplus
}
#endif

#endif /* INC_STATE_MACHINE_H_ */
