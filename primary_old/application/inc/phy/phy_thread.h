/*
 * phy_thread.h
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#ifndef INC_PHY_THREAD_H_
#define INC_PHY_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "tx_api.h"

#include "config.h"
#include "88q211x.h"
#include "lan867x.h"
#include "dp83867.h"


typedef enum {
    PHY0_DP83867 = 0,
    PHY1_88Q2112 = 1,
    PHY2_88Q2112 = 2,
    PHY3_88Q2112 = 3,
    PHY4_88Q2112 = 4,
    PHY5_88Q2112 = 5,
    PHY6_LAN8671 = 6
} phy_index_t;


extern uint8_t   phy_thread_stack[PHY_THREAD_STACK_SIZE];
extern TX_THREAD phy_thread_handle;

extern phy_handle_dp83867_t hphy0;
extern phy_handle_88q211x_t hphy1, hphy2, hphy3, hphy4, hphy5;
extern phy_handle_lan867x_t hphy6;

extern void *phy_handles[NUM_PHYS];
extern float phy_temperatures[NUM_PHYS];
extern bool  phy_temperatures_valid[NUM_PHYS];


/* Exported functions*/
phy_status_t phys_init(void);
void         phy_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_PHY_THREAD_H_ */
