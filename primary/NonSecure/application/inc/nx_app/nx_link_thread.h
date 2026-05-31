/*
 * nx_link_thread.h
 *
 *  Created on: Aug 10, 2025
 *      Author: bens1
 */

#ifndef INC_NX_LINK_THREAD_H_
#define INC_NX_LINK_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"


typedef enum {
    LINK_EVENT_LINK_CHECK = 1 << 0,
    LINK_EVENT_DHCP_SAVE  = 1 << 1
} link_event_t;


extern TX_THREAD nx_link_thread_handle;
extern uint8_t   nx_link_thread_stack[NX_LINK_THREAD_STACK_SIZE];

extern TX_EVENT_FLAGS_GROUP link_events_handle;

extern TX_TIMER link_check_timer;
extern TX_TIMER dhcp_save_timer;


void nx_link_thread_entry(uint32_t thread_input);
void link_check_timer_callback(ULONG id);
void dhcp_save_timer_callback(ULONG id);


#ifdef __cplusplus
}
#endif

#endif /* INC_NX_LINK_THREAD_H_ */
