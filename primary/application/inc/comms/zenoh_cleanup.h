/*
 * zenoh_cleanup.h
 *
 *  Created on: Sep 28, 2025
 *      Author: bens1
 */

#ifndef INC_ZENOH_ZENOH_CLEANUP_H_
#define INC_ZENOH_ZENOH_CLEANUP_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"
#include "stdatomic.h"


extern atomic_bool       zenoh_udp_multicast_groups_valid[NX_MAX_MULTICAST_GROUPS];
extern volatile uint32_t zenoh_udp_multicast_groups_list[NX_MAX_MULTICAST_GROUPS];


void zenoh_cleanup_tx(void);
void zenoh_cleanup_nx(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_ZENOH_ZENOH_CLEANUP_H_ */
