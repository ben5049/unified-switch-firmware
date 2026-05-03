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


/* Exported variables */
extern uint8_t              switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
extern TX_THREAD            switch_thread_handle;
extern atomic_uint_fast32_t sja1105_error_counter;
extern sja1105_handle_t     hsw0;
#if HW_VERSION == 5
extern sja1105_handle_t hsw1;
#endif
extern float switch_temperatures[NUM_SWITCHES];
extern bool  switch_temperatures_valid[NUM_SWITCHES];

/* Exported functions */
sja1105_status_t switch_init(void);
void             switch_thread_entry(uint32_t initial_input);


static inline sja1105_handle_t *phy_to_switch_handle(phy_index_t i) {
#if HW_VERSION == 4
    return &hsw0;
#elif HW_VERSION == 5
    switch (i) {
        case PHY0_DP83867:
        case PHY1_88Q2112:
        case PHY2_88Q2112:
        case PHY3_88Q2112:
            return &hsw1;
        case PHY4_88Q2112:
        case PHY5_88Q2112:
        case PHY6_LAN8671:
            return &hsw0;
    }
#endif
    return NULL;
}


static inline uint8_t phy_to_switch_port(phy_index_t i) {
    switch (i) {
#if HW_VERSION == 4
        case PHY0_88Q2112:
            return SW0_PORT_PHY0_88Q2112;
        case PHY1_88Q2112:
            return SW0_PORT_PHY1_88Q2112;
        case PHY2_88Q2112:
            return SW0_PORT_PHY2_88Q2112;
        case PHY3_LAN8671:
            return SW0_PORT_PHY3_LAN8671;
#elif HW_VERSION == 5
        case PHY0_DP83867:
            return SW1_PORT_PHY0_DP83867;
        case PHY1_88Q2112:
            return SW1_PORT_PHY1_88Q2112;
        case PHY2_88Q2112:
            return SW1_PORT_PHY2_88Q2112;
        case PHY3_88Q2112:
            return SW1_PORT_PHY3_88Q2112;
        case PHY4_88Q2112:
            return SW0_PORT_PHY4_88Q2112;
        case PHY5_88Q2112:
            return SW0_PORT_PHY5_88Q2112;
        case PHY6_LAN8671:
            return SW0_PORT_PHY6_LAN8671;
#endif
    }
    return -1;
}


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_THREAD_H_ */
