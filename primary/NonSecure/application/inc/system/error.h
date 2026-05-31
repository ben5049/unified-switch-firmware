/*
 * error.h
 *
 *  Created on: Mar 28, 2026
 *      Author: bens1
 */

#ifndef INC_ERROR_H_
#define INC_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_app.h"


/* Stop execution if in debug mode so the state can be inspected */
#if DEBUG
#define DEBUG_STOP(x) error_handler(x)
#else
#define DEBUG_STOP(x) (void *) (x)
#endif


void error_handler(void);
void thread_stack_error_handler(TX_THREAD *thread_ptr);


#ifdef __cplusplus
}
#endif

#endif /* INC_ERROR_H_ */
