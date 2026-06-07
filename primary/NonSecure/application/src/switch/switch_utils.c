/*
 * switch_utils.c
 *
 *  Created on: May 4, 2026
 *      Author: bens1
 */

#include "switch.h"
#include "phy.h"
#include "ptp.h"
#include "utils.h"
#include "validation.h"


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


sja1105_status_t switch_get_speed(port_index_t port, uint16_t *speed) {

    sja1105_status_t status = SJA1105_OK;
    sja1105_speed_t  speed_enum;

    status = SJA1105_PortGetSpeed(
        port_to_switch_handle(port),
        port_to_switch_port(port),
        &speed_enum);

    *speed = SJA1105_SPEED_ENUM_TO_MBPS(speed_enum);

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


sja1105_status_t switch_purge_mgmt_routes() {
    return SJA1105_ManagementRouteFreeCasc(switch_handles, true, NUM_SWITCHES);
}


static void switch_format_timestamp(int64_t timestamp_raw, NX_PTP_TIME *timestamp) {

    int64_t total_ns;
    int64_t total_sec;
    int32_t ns_remainder;

    /* Convert the hardware ticks into total nanoseconds */
    total_ns = timestamp_raw * SJA1105_NS_PER_TS_TICK;

    /* Separate total seconds from the remaining nanosecond fraction */
    total_sec    = total_ns / 1000000000LL;
    ns_remainder = (int32_t) (total_ns % 1000000000LL);

    /* Nanoseconds must be positive when seconds are greater than 0 */
    // assert((total_sec == 0) || (ns_remainder >= 0)); // TODO: re-enable?

    /* Store timestamp in the NX_PTP_TIME structure.
     * Standard PTP seconds are 48-bit. We put the upper 16 bits in
     * second_high and the lower 32 bits in second_low.
     */
    timestamp->second_high = (LONG) (total_sec >> 32);
    timestamp->second_low  = (ULONG) (total_sec & 0xFFFFFFFFULL);
    timestamp->nanosecond  = (LONG) ns_remainder;
}


/* This function assumes the switches are already synchronised */
sja1105_status_t switch_set_time_all(const NX_PTP_TIME *time) {

    sja1105_status_t switch_status = SJA1105_OK;
    tx_status_t      tx_status     = TX_SUCCESS;
    uint64_t         raw_time_new;
    uint64_t         raw_time_current;
    bool             add;

    /* Can't set the time to be negative */
    assert(time->second_high >= 0);
    assert(time->nanosecond >= 0);

    /* Calculate the new time */
    {
        uint64_t total_seconds;

        /* Combine the 48-bit PTP seconds into a single 64-bit integer */
        total_seconds = ((uint64_t) time->second_high << 32) | time->second_low;

        /* Convert seconds to 8ns ticks */
        raw_time_new = total_seconds * (S_TO_NS(1) / SJA1105_NS_PER_TS_TICK);

        /* Add the nanosecond fraction */
        raw_time_new += ((uint64_t) time->nanosecond) / SJA1105_NS_PER_TS_TICK;
    }

    /* Take the mutex to ensure time setting is atomic
     * Note: Operations are already atomic within each switch, but when
     *       measuring offsets between switches there could be glitches
     *       otherwise. */
    tx_status = tx_mutex_get(&switch_mutex_handle, PTP_CLOCK_TIMEOUT);
    if (tx_status != TX_SUCCESS) {
        switch_status = SJA1105_MUTEX_ERROR;
        return switch_status;
    }

    /* Get the current time from the main SJA1105 */
    switch_status = SJA1105_GetCurrentTime(switch_handles, &raw_time_current);
    if (switch_status != SJA1105_OK) return switch_status;

    /* Calculate the difference */
    if (raw_time_new >= raw_time_current) {
        add           = true;
        raw_time_new -= raw_time_current;
    } else {
        add          = false;
        raw_time_new = raw_time_current - raw_time_new;
    }

    /* Add the offset to all the switches */
    for (switch_index_t i = SWITCH0; i < NUM_SWITCHES; i++) {
        switch_status = SJA1105_UpdateTimestamp(&switch_handles[i], raw_time_new, add, !add);
        if (switch_status != SJA1105_OK) return switch_status;
    }

    /* Notify the MAC sync thread that there may be invalid time stamps */
    tx_status = tx_event_flags_set(&ptp_mac_sync_events_group, PTP_CLOCK_EVENT_RESET, TX_OR);
    TX_CHECK(tx_status);

    /* Release the mutex */
    tx_status = tx_mutex_put(&switch_mutex_handle);
    if (tx_status != TX_SUCCESS) {
        switch_status = SJA1105_MUTEX_ERROR;
        return switch_status;
    }

    return switch_status;
}


/* This function assumes the switches are already synchronised */
sja1105_status_t switch_add_ns_all(int32_t nanoseconds) {

    sja1105_status_t status    = SJA1105_OK;
    tx_status_t      tx_status = TX_SUCCESS;
    uint64_t         raw_time;
    bool             add;

    raw_time = ABS(nanoseconds) / SJA1105_NS_PER_TS_TICK;
    add      = nanoseconds >= 0;

    /* Take the mutex to ensure time setting is atomic
     * Note: Operations are already atomic within each switch, but when
     *       measuring offsets between switches there could be glitches
     *       otherwise. */
    tx_status = tx_mutex_get(&switch_mutex_handle, PTP_CLOCK_TIMEOUT);
    if (tx_status != TX_SUCCESS) {
        status = SJA1105_MUTEX_ERROR;
        return status;
    }

    /* Add the offset to all the switches */
    for (switch_index_t i = SWITCH0; i < NUM_SWITCHES; i++) {
        status = SJA1105_UpdateTimestamp(&switch_handles[i], raw_time, add, !add);
        if (status != SJA1105_OK) return status;
    }

    /* Release the mutex */
    tx_status = tx_mutex_put(&switch_mutex_handle);
    if (tx_status != TX_SUCCESS) {
        status = SJA1105_MUTEX_ERROR;
        return status;
    }

    return status;
}


sja1105_status_t switch_set_rate_all(uint32_t rate, const int32_t *corrections) {

    sja1105_status_t status    = SJA1105_OK;
    tx_status_t      tx_status = TX_SUCCESS;
    uint32_t         rate_corrected;
    static uint32_t  rates_previous[NUM_SWITCHES] = {0};

    /* Take the mutex to ensure rate setting is atomic
     * Note: Operations are already atomic within each switch, but delays
     *       between the setting operations leads to timing jitter */
    tx_status = tx_mutex_get(&switch_mutex_handle, PTP_CLOCK_TIMEOUT);
    if (tx_status != TX_SUCCESS) {
        status = SJA1105_MUTEX_ERROR;
        return status;
    }

    /* Apply the rate to all the switches */
    for (switch_index_t i = SWITCH0; i < NUM_SWITCHES; i++) {

        /*  Apply per switch corrections */
        if (corrections == NULL) {
            rate_corrected = rate;
        } else if ((corrections[i] >= 0) && ((UINT32_MAX - rate) < (uint32_t) corrections[i])) {
            rate_corrected = UINT32_MAX;
        } else if ((corrections[i] < 0) && ((uint32_t) (-corrections[i]) > rate)) {
            rate_corrected = 0;
        } else {
            rate_corrected = rate + corrections[i];
        }

        /* Set the rate */
        if (rates_previous[i] != rate_corrected) {
            status = SJA1105_SetPTPClockRate(
                &switch_handles[i],
                rate_corrected);
            if (status != SJA1105_OK) return status;
            rates_previous[i] = rate_corrected;
        }

        /* Inject delay to see if the switch sync thread can fix it */
        if (i != (NUM_SWITCHES - 1)) {
            VAL_FAULT_CHANCE(SWITCH_UTILS, RATE_SET_PREEMPT, VAL_1_IN_100, tx_thread_sleep_ms(10));
        }
    }

    /* Release the mutex */
    tx_status = tx_mutex_put(&switch_mutex_handle);
    if (tx_status != TX_SUCCESS) {
        status = SJA1105_MUTEX_ERROR;
        return status;
    }

    return status;
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


sja1105_status_t switch_parse_and_free_meta_frame(NX_PACKET *packet, bool get_timestamp, port_index_t *port, NX_PTP_TIME *timestamp) {

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

    /* Parse the META frame and get the timestamp */
    if (get_timestamp && (timestamp != NULL)) {
        status = SJA1105_GetIngressTimestamp(switch_handles, packet->nx_packet_prepend_ptr + header_size, &switch_id, &src_port, &timestamp_raw);
        if (status != SJA1105_OK) goto end;

        /* Turn the 8ns timestamp to seconds and nanoseconds */
        switch_format_timestamp(timestamp_raw, timestamp);
    }

    /* Just parse the META frame */
    else {
        status = SJA1105_ParseMETAFrame(packet->nx_packet_prepend_ptr + header_size, &switch_id, &src_port, NULL);
        if (status != SJA1105_OK) goto end;
    }

    /* Get the port */
    if (port != NULL) *port = switch_id_port_to_port(switch_id, src_port);

end:

    /* Release the META frame and return */
    if (nx_packet_release(packet) != NX_SUCCESS) status = SJA1105_ERROR;
    return status;
}


#if NUM_SWITCHES > 1

sja1105_status_t switch_get_timestamp_offsets(switch_index_t a, switch_index_t b, NX_PTP_TIME *offset) {

    sja1105_status_t status = SJA1105_OK;
    int64_t          offset_internal;

    /* Get the offset */
    status = SJA1105_GetTimestampOffset(&switch_handles[a], &switch_handles[b], &offset_internal);
    if (status != SJA1105_OK) return status;

    /* Format into PTP time struct */
    switch_format_timestamp(offset_internal, offset);

    return status;
}

#endif
