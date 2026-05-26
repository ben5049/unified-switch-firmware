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


sja1105_status_t switch_byte_pool_init_all() {
#if HW_VERSION == 4
    return switch_byte_pool_init(SWCH_POOL0);
#elif HW_VERSION == 5
    return switch_byte_pool_init(SWCH_POOL0 | SWCH_POOL1);
#endif
}


sja1105_status_t switch_disable_forwarding(port_index_t port) {
    return SJA1105_PortSetForwarding(
        port_to_switch_handle(port),
        port_to_switch_port(port),
        false);
}


sja1105_status_t switch_enable_forwarding(port_index_t port) {
    return SJA1105_PortSetForwarding(
        port_to_switch_handle(port),
        port_to_switch_port(port),
        true);
}


bool switch_port_is_dynamic(port_index_t port) {

    sja1105_handle_t *switch_handle = port_to_switch_handle(port);
    uint8_t           switch_port   = port_to_switch_port(port);

    return switch_handle->config->ports[switch_port].speed == SJA1105_SPEED_DYNAMIC;
}


/* Speed in mbps */
sja1105_status_t switch_update_speed(port_index_t port, uint16_t speed) {

    if (switch_port_is_dynamic(port)) {
        return SJA1105_PortSetSpeed(
            port_to_switch_handle(port),
            port_to_switch_port(port),
            SJA1105_SPEED_MBPS_TO_ENUM(speed));
    }

    else {
        return SJA1105_OK;
    }
}


/* Update the switch port speed from the PHY speed */
sja1105_status_t switch_update_speed_from_phy(phy_index_t phy) {

    sja1105_status_t status = SJA1105_OK;
    phy_speed_t      speed;

    assert(phy < NUM_PHYS);

    /* For dynamic speed ports get the PHY speed and set the switch port to that speed */
    if (switch_port_is_dynamic(phy)) {

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


/* Given a port index, create a management route to that port from the host port */
sja1105_status_t switch_create_mgmt_route(port_index_t port, const uint8_t *dst_addr, bool takets, uint8_t tsreg, uint8_t *depth, sja1105_mgmt_route_free_callback_t free_callback, void *callback_context) {

    sja1105_status_t status = SJA1105_OK;

    status = SJA1105_ManagementRouteCreateCascSingle(
        switch_handles,
        port_to_switch_handle(port)->config->switch_id,
        port_to_switch_port(port),
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
    return SJA1105_ManagementRouteFreeCasc(switch_handles, false, depth);
}


void switch_format_timestamp(uint64_t timestamp_raw, NX_PTP_TIME *timestamp) {

    uint64_t total_ns;
    uint64_t total_sec;

    /* Convert the hardware ticks (8ns per tick) into total nanoseconds */
    total_ns = timestamp_raw * 8;

    /* Separate total seconds from the remaining nanosecond fraction */
    total_sec = total_ns / 1000000000ULL;

    /* Store timestamp in the NX_PTP_TIME structure.
     * Standard PTP seconds are 48-bit. We put the upper 16 bits in
     * second_high and the lower 32 bits in second_low.
     */
    timestamp->second_high = (uint32_t) (total_sec >> 32);
    timestamp->second_low  = (uint32_t) (total_sec & 0xFFFFFFFFULL);
    timestamp->nanosecond  = (uint32_t) (total_ns % 1000000000ULL);
}


sja1105_status_t switch_get_egress_timestamp(port_index_t port, uint8_t tsreg, NX_PTP_TIME *timestamp) {

    sja1105_status_t status = SJA1105_OK;
    uint64_t         timestamp_raw;

    /* Get the egress timestamp in 8ns intervals */
    status = SJA1105_GetEgressTimestamp(
        port_to_switch_handle(port),
        port_to_switch_port(port),
        tsreg,
        &timestamp_raw);
    if (status != SJA1105_OK) return status;

    /* Turn the 8ns timestamp to seconds and nanoseconds */
    switch_format_timestamp(timestamp_raw, timestamp);

    return status;
}


sja1105_status_t switch_parse_and_free_meta_frame(NX_PACKET *packet, port_index_t *port, NX_PTP_TIME *timestamp) {

    sja1105_status_t status = SJA1105_OK;

    UINT header_size;

    uint8_t  switch_id;
    uint8_t  src_port;
    uint64_t timestamp_raw;

    /* Get info from the header */
    if (nx_link_ethernet_header_parse(packet, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &header_size) != NX_SUCCESS) {
        status = SJA1105_ERROR;
        goto end;
    }

    /* Parse META frame and get raw timestamp */
    status = SJA1105_GetIngressTimestamp(switch_handles, packet->nx_packet_prepend_ptr + header_size, &switch_id, &src_port, &timestamp_raw);
    if (status != SJA1105_OK) goto end;

    /* Turn the 8ns timestamp to seconds and nanoseconds */
    if (timestamp != NULL) switch_format_timestamp(timestamp_raw, timestamp);

    /* Get the port */
    if (port != NULL) *port = switch_id_port_to_port(switch_id, src_port);

end:

    /* Release the META frame packet and return */
    if (nx_packet_release(packet) != NX_SUCCESS) status = SJA1105_ERROR;
    return status;
}
