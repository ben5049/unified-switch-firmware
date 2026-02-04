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


#if HW_VERSION == 4
#include "swv4_sja1105_static_config_default.h"
#define SWITCH0_CONFIG      swv4_sja1105_static_config_default
#define SWITCH0_CONFIG_SIZE SWV4_SJA1105_STATIC_CONFIG_DEFAULT_SIZE
#elif HW_VERSION == 5
#include "swv5_0_sja1105_static_config_default.h"
#include "swv5_1_sja1105_static_config_default.h"
#define SWITCH0_CONFIG      swv5_0_sja1105_static_config_default
#define SWITCH0_CONFIG_SIZE SWV5_0_SJA1105_STATIC_CONFIG_DEFAULT_SIZE
#define SWITCH1_CONFIG      swv5_1_sja1105_static_config_default
#define SWITCH1_CONFIG_SIZE SWV5_1_SJA1105_STATIC_CONFIG_DEFAULT_SIZE
#endif

/* Enums */

typedef enum {
    SWITCH0 = 0,
    SWITCH1 = 1
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


/* Exported variables */
extern uint8_t              switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
extern TX_THREAD            switch_thread_handle;
extern atomic_uint_fast32_t sja1105_error_counter;
extern sja1105_handle_t     hsw0
#if HW_VERSION == 5
extern sja1105_handle_t     hsw1;
#endif
extern float                switch_temperature;
extern bool                 switch_temperature_valid;


/* Exported functions*/
sja1105_status_t switch_init(sja1105_handle_t *dev);
void             switch_thread_entry(uint32_t initial_input);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_THREAD_H_ */
