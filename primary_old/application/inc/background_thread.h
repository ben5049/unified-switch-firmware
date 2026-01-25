/*
 * background_thread.h
 *
 *  Created on: Oct 5, 2025
 *      Author: bens1
 */

#ifndef INC_BACKGROUND_THREAD_H_
#define INC_BACKGROUND_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"

#include "config.h"


extern TX_THREAD background_thread_handle;
extern uint8_t   background_thread_stack[BACKGROUND_THREAD_STACK_SIZE];


void background_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_BACKGROUND_THREAD_H_ */
