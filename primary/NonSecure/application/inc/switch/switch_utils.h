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


phy_status_t switch_update_speed_from_phy(phy_index_t phy);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_UTILS_H_ */
