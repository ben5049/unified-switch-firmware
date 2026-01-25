/*
 * stp_thread.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_STP_THREAD_H_
#define INC_STP_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "tx_api.h"

#include "config.h"
#include "nx_app.h"


#define STP_ALL_EVENTS                    ((ULONG) 0xffffffff)
#define STP_BPDU_REC_EVENT                (1 < 0)
#define STP_PORT0_LINK_STATE_CHANGE_EVENT (1 < 1)
#define STP_PORT1_LINK_STATE_CHANGE_EVENT (1 < 2)
#define STP_PORT2_LINK_STATE_CHANGE_EVENT (1 < 3)
#define STP_PORT3_LINK_STATE_CHANGE_EVENT (1 < 4)
#define STP_PORT4_LINK_STATE_CHANGE_EVENT (1 < 5)


typedef struct STP_BRIDGE STP_BRIDGE;


/* Exported variables */
extern uint8_t              stp_thread_stack[STP_THREAD_STACK_SIZE];
extern TX_THREAD            stp_thread_handle;
extern TX_EVENT_FLAGS_GROUP stp_events_handle;

/* Exported functions*/
void stp_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_STP_THREAD_H_ */
