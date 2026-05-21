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


static inline sja1105_handle_t *phy_to_switch_handle(phy_index_t phy) {
#if HW_VERSION == 4
    return &switch_handles[0];
#elif HW_VERSION == 5
    switch (phy) {
        case PHY0_DP83867:
        case PHY1_88Q2112:
        case PHY2_88Q2112:
        case PHY3_88Q2112:
            return &switch_handles[1];
        case PHY4_88Q2112:
        case PHY5_88Q2112:
        case PHY6_LAN8671:
            return &switch_handles[0];
    }
#endif
    return NULL;
}


static inline uint8_t phy_to_switch_port(phy_index_t phy) {
    switch (phy) {
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


static inline phy_index_t switch_id_port_to_phy(switch_index_t switch_id, uint8_t port) {
    switch (switch_id) {
        case SWITCH0: {
            switch (port) {
#if HW_VERSION == 4
                case SW0_PORT_PHY0_88Q2112:
                    return PHY0_88Q2112;
                case SW0_PORT_PHY1_88Q2112:
                    return PHY1_88Q2112;
                case SW0_PORT_PHY2_88Q2112:
                    return PHY2_88Q2112;
                case SW0_PORT_PHY3_LAN8671:
                    return PHY3_LAN8671;
                default:
                    return -1;
#elif HW_VERSION == 5
                case SW0_PORT_PHY4_88Q2112:
                    return PHY4_88Q2112;
                case SW0_PORT_PHY5_88Q2112:
                    return PHY5_88Q2112;
                case SW0_PORT_PHY6_LAN8671:
                    return PHY6_LAN8671;
                default:
                    return -1;
            }
        }
        case SWITCH1: {
            switch (port) {

                case SW1_PORT_PHY0_DP83867:
                    return PHY0_DP83867;
                case SW1_PORT_PHY1_88Q2112:
                    return PHY1_88Q2112;
                case SW1_PORT_PHY2_88Q2112:
                    return PHY2_88Q2112;
                case SW1_PORT_PHY3_88Q2112:
                    return PHY3_88Q2112;
#endif
            }
        }
        default: {
            return -1;
        }
    }
}


static inline sja1105_status_t switch_byte_pool_init_all() {
#if HW_VERSION == 4
    return switch_byte_pool_init(SWCH_POOL0);
#elif HW_VERSION == 5
    return switch_byte_pool_init(SWCH_POOL0 | SWCH_POOL1);
#endif
}


static inline sja1105_status_t switch_disable_forwarding(phy_index_t phy) {
    return SJA1105_PortSetForwarding(
        phy_to_switch_handle(phy),
        phy_to_switch_port(phy),
        false);
}


static inline sja1105_status_t switch_enable_forwarding(phy_index_t phy) {
    return SJA1105_PortSetForwarding(
        phy_to_switch_handle(phy),
        phy_to_switch_port(phy),
        true);
}


static inline bool switch_port_dynamic(phy_index_t phy) {

    sja1105_handle_t *switch_handle = phy_to_switch_handle(phy);
    uint8_t           switch_port   = phy_to_switch_port(phy);

    return switch_handle->config->ports[switch_port].speed == SJA1105_SPEED_DYNAMIC;
}


/* Speed in mbps */
static inline sja1105_status_t switch_update_speed(phy_index_t phy, uint16_t speed) {

    if (switch_port_dynamic(phy)) {
        return SJA1105_PortSetSpeed(
            phy_to_switch_handle(phy),
            phy_to_switch_port(phy),
            SJA1105_SPEED_MBPS_TO_ENUM(speed));
    }

    else {
        return SJA1105_OK;
    }
}


sja1105_status_t switch_update_speed_from_phy(phy_index_t port);
sja1105_status_t switch_create_mgmt_route(phy_index_t port, const uint8_t *dst_addr, bool takets, uint8_t tsreg, uint8_t *depth, sja1105_mgmt_route_free_callback_t free_callback, void *callback_context);
sja1105_status_t switch_free_mgmt_route(uint8_t depth);
void             switch_format_timestamp(uint64_t timestamp_raw, NX_PTP_TIME *timestamp);
sja1105_status_t switch_get_egress_timestamp(phy_index_t port, uint8_t tsreg, NX_PTP_TIME *timestamp);
sja1105_status_t switch_parse_meta_frame(NX_PACKET *packet, phy_index_t *port, NX_PTP_TIME *timestamp);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_UTILS_H_ */
