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


/* Enums */

typedef enum {
    SWITCH0 = 0,
    SWITCH1 = 1
} switch_index_t;

typedef enum {
    SW0_PORT_SW1          = 0x0,
    SW0_PORT_PHY4_88Q2112 = 0x1,
    SW0_PORT_PHY5_88Q2112 = 0x2,
    SW0_PORT_PHY6_LAN8671 = 0x3,
    SW0_PORT_HOST         = 0x4,
} switch0_port_index_t;

typedef enum {
    SW1_PORT_PHY0_DP83867 = 0x0,
    SW1_PORT_PHY1_88Q2112 = 0x1,
    SW1_PORT_PHY2_88Q2112 = 0x2,
    SW1_PORT_PHY3_88Q2112 = 0x3,
    SW1_PORT_SW0          = 0x4,
} switch1_port_index_t;


/* Exported variables */
extern uint8_t              switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
extern TX_THREAD            switch_thread_handle;
extern atomic_uint_fast32_t sja1105_error_counter;
extern sja1105_handle_t     hsw0, hsw1;
extern float                switch_temperature;
extern bool                 switch_temperature_valid;


/* Exported functions*/
sja1105_status_t switch_init(sja1105_handle_t *dev);
void             switch_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_THREAD_H_ */
