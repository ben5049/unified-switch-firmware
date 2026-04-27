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
#if HW_VERSION == 4
    PHY0_88Q2112 = 0,
    PHY1_88Q2112 = 1,
    PHY2_88Q2112 = 2,
    PHY3_LAN8671 = 3
#elif HW_VERSION == 5
    PHY0_DP83867 = 0,
    PHY1_88Q2112 = 1,
    PHY2_88Q2112 = 2,
    PHY3_88Q2112 = 3,
    PHY4_88Q2112 = 4,
    PHY5_88Q2112 = 5,
    PHY6_LAN8671 = 6
#endif
} phy_index_t;

typedef enum {
    PHY_STATE_UNINITIALISED,
    PHY_STATE_INITIALISED,
    PHY_STATE_WAIT_FOR_LINK,
    PHY_STATE_CONNECTED,
    PHY_STATE_RECONNECT,
    PHY_STATE_SLEEP_START,
    PHY_STATE_SLEEP_STOP,
    PHY_STATE_SELF_TEST_1,
    PHY_STATE_SELF_TEST_2,
    PHY_STATE_SELF_TEST_3,
    PHY_STATE_SELF_TEST_4,
    PHY_STATE_SELF_TEST_5,
    PHY_STATE_SELF_TEST_6,
    PHY_STATE_SELF_TEST_7,
    PHY_STATE_SELF_TEST_8,
    PHY_STATE_SELF_TEST_9,
    PHY_STATE_SELF_TEST_10,
    PHY_STATE_ERROR_RECOVERABLE,
    PHY_STATE_ERROR_UNRECOVERABLE,
    PHY_STATE_INVALID,
} phy_connection_state_t;

/* Struct used as the PHY callback context */
typedef struct {
    phy_index_t index;

    phy_connection_state_t connection_state;
    uint32_t               next_update_time; /* Time */
    uint8_t                link_attempts;
    uint32_t               last_self_test_time;
} phy_info_t;


extern uint8_t   phy_thread_stack[PHY_THREAD_STACK_SIZE];
extern TX_THREAD phy_thread_handle;

#if HW_VERSION == 4
extern phy_handle_88q211x_t hphy0, hphy1, hphy2;
extern phy_handle_lan867x_t hphy3;
#elif HW_VERSION == 5
extern phy_handle_dp83867_t hphy0;
extern phy_handle_88q211x_t hphy1, hphy2, hphy3, hphy4, hphy5;
extern phy_handle_lan867x_t hphy6;
#endif

extern void *phy_handles[NUM_PHYS];
extern float phy_temperatures[NUM_PHYS];
extern bool  phy_temperatures_valid[NUM_PHYS];


/* Exported functions*/
phy_status_t phys_init(void);
void         phy_thread_entry(uint32_t initial_input);
phy_status_t phy_state_update_poll(phy_handle_base_t *hphy, uint32_t current_time);
phy_status_t phy_state_update_interrupt(phy_handle_base_t *hphy, uint32_t current_time);


#ifdef __cplusplus
}
#endif

#endif /* INC_PHY_THREAD_H_ */
