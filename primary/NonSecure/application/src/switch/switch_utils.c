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
sja1105_status_t switch_create_mgmt_route(phy_index_t phy, const uint8_t *dst_addr, bool takets, uint8_t tsreg, uint8_t *depth, sja1105_mgmt_route_free_callback_t free_callback, void *callback_context) {

    sja1105_status_t status = SJA1105_OK;

    status = SJA1105_ManagementRouteCreateCascSingle(
        &switch_handles[0],
        phy_to_switch_handle(phy)->config->switch_id,
        phy_to_switch_port(phy),
        dst_addr,
        takets,
        tsreg,
        depth,
        free_callback,
        callback_context);

    return status;
}


sja1105_status_t switch_free_mgmt_route(uint8_t depth) {
    if (depth == 0) return SJA1105_OK;
    return SJA1105_ManagementRouteFreeCasc(&switch_handles[0], false, depth);
}


sja1105_status_t switch_get_egress_timestamp(phy_index_t phy, uint8_t tsreg, NX_PTP_TIME *timestamp) {

    sja1105_status_t status = SJA1105_OK;
    uint64_t         egress_timestamp;
    uint64_t         total_ns;
    uint64_t         total_sec;

    /* Get the egress timestamp in 8ns intervals */
    status = SJA1105_GetEgressTimestamp(
        phy_to_switch_handle(phy),
        phy_to_switch_port(phy),
        tsreg,
        &egress_timestamp);
    if (status != SJA1105_OK) return status;

    /* Convert the hardware ticks (8ns per tick) into total nanoseconds */
    total_ns = egress_timestamp * 8;

    /* Separate total seconds from the remaining nanosecond fraction */
    total_sec = total_ns / 1000000000ULL;

    /* Store timestamp in the NX_PTP_TIME structure.
     * Standard PTP seconds are 48-bit. We put the upper 16 bits in
     * second_high and the lower 32 bits in second_low.
     */
    timestamp->second_high = (uint32_t) (total_sec >> 32);
    timestamp->second_low  = (uint32_t) (total_sec & 0xFFFFFFFFULL);
    timestamp->nanosecond  = (uint32_t) (total_ns % 1000000000ULL);

    return status;
}
