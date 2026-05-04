/*
 * switch_utils.c
 *
 *  Created on: May 4, 2026
 *      Author: bens1
 */


#include "switch_utils.h"
#include "phy_common.h"
#include "phy_thread.h"


/* Update the switch port speed from the PHY speed */
phy_status_t switch_update_speed_from_phy(phy_index_t phy) {

    phy_status_t status = PHY_OK;
    phy_speed_t  speed;

    /* For dynamic speed ports get the PHY speed and set the switch port to that speed */
    if (switch_port_dynamic(phy)) {

        /* Get PHY speed */
        status = PHY_GetSpeed(phy_handles[phy], &speed);
        if ((status != PHY_OK) && (status != PHY_NOT_IMPLEMENTED_ERROR)) goto end; // TODO: remove PHY_NOT_IMPLEMENTED_ERROR when implemented

        /* Set switch port speed */
        if (switch_update_speed(phy, PHY_SPEED_ENUM_TO_MBPS(speed)) != SJA1105_OK) {
            status = PHY_ERROR;
            goto end;
        }
    }

end:

    return status;
}
