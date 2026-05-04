/*
 * switch.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_SWITCH_THREAD_H_
#define INC_SWITCH_THREAD_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"
#include "stdint.h"
#include "stdatomic.h"
#include "hal.h"

#include "config.h"
#include "sja1105.h"
#include "phy_thread.h"


/* Enums */

typedef enum {
    SWITCH0 = 0,
#if HW_VERSION == 5
    SWITCH1 = 1
#endif
} switch_index_t;

typedef enum {

#if HW_VERSION == 4
    SW0_PORT_PHY0_88Q2112 = 0x0,
    SW0_PORT_PHY1_88Q2112 = 0x1,
    SW0_PORT_PHY2_88Q2112 = 0x2,
    SW0_PORT_PHY3_LAN8671 = 0x3,
#elif HW_VERSION == 5
    SW0_PORT_SW1          = 0x0,
    SW0_PORT_PHY4_88Q2112 = 0x1,
    SW0_PORT_PHY5_88Q2112 = 0x2,
    SW0_PORT_PHY6_LAN8671 = 0x3,
#endif
    SW0_PORT_HOST = 0x4,
} switch0_port_index_t;

#if HW_VERSION == 5
typedef enum {
    SW1_PORT_PHY0_DP83867 = 0x0,
    SW1_PORT_PHY1_88Q2112 = 0x1,
    SW1_PORT_PHY2_88Q2112 = 0x2,
    SW1_PORT_PHY3_88Q2112 = 0x3,
    SW1_PORT_SW0          = 0x4,
} switch1_port_index_t;
#endif

/* Struct used as the switch callback context */
typedef struct {
    switch_index_t index;

    float temperature;
    bool  temperature_valid;

    TX_INTERRUPT_SAVE_AREA; /* Stores the context for critical sections */
} switch_info_t;


/* Exported variables */
extern uint8_t          switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
extern TX_THREAD        switch_thread_handle;
extern sja1105_handle_t switch_handles[NUM_SWITCHES];
extern switch_info_t    switch_info[NUM_SWITCHES];

/* Exported functions */
sja1105_status_t switch_init(void);
void             switch_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_THREAD_H_ */
