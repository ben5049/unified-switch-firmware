/*
 * switch_utils.h
 *
 *  Created on: May 3, 2026
 *      Author: bens1
 */

#ifndef INC_SWITCH_UTILS_H_
#define INC_SWITCH_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "sja1105.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "phy_common.h"
#include "error.h"


#define SWITCH_CHECK(status)                    \
    if ((status) != SJA1105_OK) error_handler()


static inline sja1105_handle_t *port_to_switch_handle(port_index_t port) {
#if HW_VERSION == 4
    return &switch_handles[0];
#elif HW_VERSION == 5
    switch (port) {
        case PORT0:
        case PORT1:
        case PORT2:
        case PORT3:
            return &switch_handles[SWITCH1];
        case PORT4:
        case PORT5:
        case PORT6:
        case PORT_HOST:
            return &switch_handles[SWITCH0];
        default:
            error_handler();
    }
#endif
    return NULL;
}


static inline uint8_t port_to_switch_port(port_index_t port) {
    switch (port) {
#if HW_VERSION == 4
        case PORT0:
            return SW0_PORT_PHY0_88Q2112;
        case PORT1:
            return SW0_PORT_PHY1_88Q2112;
        case PORT2:
            return SW0_PORT_PHY2_88Q2112;
        case PORT3:
            return SW0_PORT_PHY3_LAN8671;
#elif HW_VERSION == 5
        case PORT0:
            return SW1_PORT_PHY0_DP83867;
        case PORT1:
            return SW1_PORT_PHY1_88Q2112;
        case PORT2:
            return SW1_PORT_PHY2_88Q2112;
        case PORT3:
            return SW1_PORT_PHY3_88Q2112;
        case PORT4:
            return SW0_PORT_PHY4_88Q2112;
        case PORT5:
            return SW0_PORT_PHY5_88Q2112;
        case PORT6:
            return SW0_PORT_PHY6_LAN8671;
#endif
        case PORT_HOST:
            return SW0_PORT_HOST;
        default:
            error_handler();
    }
    return -1;
}


static inline port_index_t switch_id_port_to_port(switch_index_t switch_id, uint8_t port) {
    switch (switch_id) {
        case SWITCH0: {
            switch (port) {
#if HW_VERSION == 4
                case SW0_PORT_PHY0_88Q2112:
                    return PORT0;
                case SW0_PORT_PHY1_88Q2112:
                    return PORT1;
                case SW0_PORT_PHY2_88Q2112:
                    return PORT2;
                case SW0_PORT_PHY3_LAN8671:
                    return PORT3;
#elif HW_VERSION == 5
                case SW0_PORT_PHY4_88Q2112:
                    return PORT4;
                case SW0_PORT_PHY5_88Q2112:
                    return PORT5;
                case SW0_PORT_PHY6_LAN8671:
                    return PORT6;
#endif
                case SW0_PORT_HOST:
                    return PORT_HOST;
                default:
                    error_handler();
            }
        }

#if HW_VERSION == 5

        case SWITCH1: {
            switch (port) {
                case SW1_PORT_PHY0_DP83867:
                    return PORT0;
                case SW1_PORT_PHY1_88Q2112:
                    return PORT1;
                case SW1_PORT_PHY2_88Q2112:
                    return PORT2;
                case SW1_PORT_PHY3_88Q2112:
                    return PORT3;
                default:
                    error_handler();
            }
        }

#endif

        default:
            error_handler();
    }
    return -1;
}

sja1105_status_t switch_byte_pool_init_all();

sja1105_status_t switch_disable_forwarding(port_index_t port);
sja1105_status_t switch_enable_forwarding(port_index_t port);

bool             switch_port_is_dynamic(port_index_t port);
sja1105_status_t switch_update_speed(port_index_t port, uint16_t speed);
sja1105_status_t switch_update_speed_from_phy(phy_index_t phy);
sja1105_status_t switch_get_speed(port_index_t port, uint16_t *speed);

sja1105_status_t switch_create_mgmt_route(port_index_t port, const uint8_t *dst_addr, bool takets, uint8_t tsreg, uint8_t *depth, sja1105_mgmt_route_free_callback_t free_callback, void *callback_context);
sja1105_status_t switch_free_mgmt_route(uint8_t depth);

sja1105_status_t switch_get_time(switch_index_t i, NX_PTP_TIME *time);
sja1105_status_t switch_set_time_all(NX_PTP_TIME *time);
sja1105_status_t switch_get_egress_timestamp(port_index_t port, uint8_t tsreg, NX_PTP_TIME *timestamp);
sja1105_status_t switch_parse_and_free_meta_frame(NX_PACKET *packet, bool get_timestamp, port_index_t *port, NX_PTP_TIME *timestamp);
#if NUM_SWITCHES > 1
sja1105_status_t switch_get_timestamp_offsets(switch_index_t a, switch_index_t b, NX_PTP_TIME *offset);
#endif


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_UTILS_H_ */
