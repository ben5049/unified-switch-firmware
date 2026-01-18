/*
 * stp_callbacks.h
 *
 *  Created on: Aug 2, 2025
 *      Author: bens1
 */

#ifndef INC_STP_STP_CALLBACKS_H_
#define INC_STP_STP_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"
#include "hal.h"
//#include "stp.h" TODO: remove

#include "nx_stp.h"


typedef struct STP_CALLBACKS STP_CALLBACKS;

extern const STP_CALLBACKS stp_callbacks;

UINT stp_byte_pool_init(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_STP_STP_CALLBACKS_H_ */
