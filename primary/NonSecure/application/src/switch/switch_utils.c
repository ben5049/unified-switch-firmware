/*
 * switch_utils.c
 *
 *  Created on: May 4, 2026
 *      Author: bens1
 */

#include "sja1105.h"

#include "switch_utils.h"
#include "phy_common.h"
#include "phy_thread.h"


/* Update the switch port speed from the PHY speed */
sja1105_status_t switch_update_speed_from_phy(phy_index_t phy) {

    sja1105_status_t status = SJA1105_OK;
    phy_speed_t      speed;

    /* For dynamic speed ports get the PHY speed and set the switch port to that speed */
    if (switch_port_dynamic(phy)) {

        /* Get PHY speed */
        if (PHY_GetSpeed(phy_handles[phy], &speed) != PHY_OK) {
            status = SJA1105_ERROR;
            goto end;
        }

        /* Set switch port speed */
        status = switch_update_speed(phy, PHY_SPEED_ENUM_TO_MBPS(speed));
        if (status != SJA1105_OK) goto end;
    }

end:

    return status;
}


/* Given a PHY index, create a management route to that PHY from the host port */
sja1105_status_t switch_create_mgmt_route(phy_index_t phy, const uint8_t *dst_addr, bool takets, bool tsreg) {

    sja1105_status_t status = SJA1105_OK;

    status = SJA1105_ManagementRouteCreateCascSingle(
        switch_handles,
        phy_to_switch_handle(phy)->config->switch_id,
        phy_to_switch_port(phy),
        dst_addr,
        takets,
        tsreg,
        NULL);

    return status;
}
