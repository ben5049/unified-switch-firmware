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
#include "tx_api.h"
#include "nx_api.h"
#include "config.h"


extern TX_THREAD nx_link_thread_handle;
extern uint8_t   nx_link_thread_stack[NX_LINK_THREAD_STACK_SIZE];


void nx_link_thread_entry(uint32_t thread_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_NX_LINK_THREAD_H_ */
