/*
 * switch_diagnostics.c
 *
 *  Created on: Oct 4, 2025
 *      Author: bens1
 */

#include "zenoh-pico.h"
#include "pb_encode.h"
#include "switch.pb.h"

#include "app.h"
#include "switch_thread.h"
#include "switch_diagnostics.h"
#include "encodings.h"
#include "state_machine.h"
#include "comms_thread.h"
#include "phy_thread.h"


#define SWITCH_STATS_BUFFER_SIZE (256)


static pb_ostream_t       stream;
static z_owned_encoding_t stats_encoding;
static uint8_t            switch_stats_buffer[SWITCH_STATS_BUFFER_SIZE];
static SwitchDiag         switch_diag = SwitchDiag_init_default;


bool switch_stats_port_callback(pb_ostream_t *stream, const pb_field_t *field, void * const *arg) {

    // TODO: This function needs a complete rework to handle the cascaded setup
    // error_handler();

    // /* Read the port high level statistics counters from the switch chip */
    // sja1105_statistics_t switch_stats;
    // if (SJA1105_ReadStatistics(&hsw0, &switch_stats) != SJA1105_OK) error_handler();

    // /* Go through each port */
    // for (uint_fast8_t port_index = 0; port_index < SJA1105_NUM_PORTS; port_index++) {

    //     /* Create the struct to store the port stats */
    //     PortDiag port       = PortDiag_init_default;
    //     bool     forwarding = false;

    //     /* Assign the port high level statistics */
    //     port.rx_bytes           = switch_stats.rx_bytes[port_index];
    //     port.has_rx_bytes       = true;
    //     port.tx_bytes           = switch_stats.tx_bytes[port_index];
    //     port.has_tx_bytes       = true;
    //     port.dropped_frames     = switch_stats.dropped_frames[port_index];
    //     port.has_dropped_frames = true;

    //     /* Get and assign the port state */
    //     if (SJA1105_PortGetForwarding(&hsw0, port_index, &forwarding) != SJA1105_OK) error_handler();
    //     port.state = forwarding ? PortState_FORWARDING : PortState_DISABLED;

    //     /* Assign the PHY temperature */
    //     if (port_index == SW0_PORT_HOST) {
    //         port.has_phy_temp = false; /* TODO: Get this value from the DTS */
    //     } else {
    //         port.phy_temp     = phy_temperatures[port_index];
    //         port.has_phy_temp = phy_temperatures_valid[port_index];
    //     }

    //     /* Encode this sub message */
    //     if (!pb_encode_tag_for_field(stream, field)) error_handler();
    //     if (!pb_encode_submessage(stream, PortDiag_fields, &port)) error_handler();
    // }

    return true;
}


sja1105_status_t init_switch_diagnostics() {
    switch_diag.ports.funcs.encode = &switch_stats_port_callback;
    switch_diag.ports.arg          = NULL;
    return SJA1105_OK;
}


sja1105_status_t publish_switch_diagnostics(uint32_t current_time) {

    sja1105_status_t sja_status = SJA1105_OK;
    tx_status_t      tx_status  = TX_SUCCESS;
    _z_res_t         z_status   = Z_OK;
    uint32_t         flags;

    /* Check if publishing stats is allowed */
    tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_ZENOH_CONNECTED, TX_OR, &flags, TX_NO_WAIT);
    if (tx_status == TX_SUCCESS) {

        /* Reset variables */
        stream = pb_ostream_from_buffer(switch_stats_buffer, SWITCH_STATS_BUFFER_SIZE);
        z_owned_bytes_t           payload;
        z_publisher_put_options_t options;

        /* Get the stats */
        switch_diag.timestamp.seconds     = current_time / 1000;             /* TODO: Get from PTP */
        switch_diag.timestamp.nanoseconds = (current_time % 1000) * 1000000; /* TODO: Get from PTP */
        switch_diag.has_timestamp         = true;
        switch_diag.temp                  = switch_temperatures[0];
        switch_diag.has_temp              = switch_temperatures_valid[0];

        /* Encode the message */
        if (!pb_encode(&stream, SwitchDiag_fields, &switch_diag)) {
            sja_status = SJA1105_ERROR;
            return sja_status;
        }

        /* Convert into a Zenoh payload */
        z_status = z_bytes_from_static_buf(&payload, switch_stats_buffer, stream.bytes_written);
        if (z_status < Z_OK) tx_status = zenoh_disconnected(false);
        if (tx_status != TX_SUCCESS) error_handler();

        /* Check if publishing stats is still allowed */
        tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_ZENOH_CONNECTED, TX_OR, &flags, TX_NO_WAIT);
        if (tx_status == TX_SUCCESS) {

            /* Publish the message */
            z_publisher_put_options_default(&options);
            z_status = z_encoding_from_str(&stats_encoding, ENCODING_SWITCH_STATS);
            if (z_status < Z_OK) tx_status = zenoh_disconnected(false);
            if (tx_status != TX_SUCCESS) error_handler();
            options.encoding = z_move(stats_encoding);
            z_status         = z_publisher_put(z_loan(stats_pub), z_move(payload), &options);
            if (z_status < Z_OK) tx_status = zenoh_disconnected(false);
            if (tx_status != TX_SUCCESS) error_handler();
        }

        /* Error occured */
        else if (tx_status != TX_NO_EVENTS) {
            sja_status = SJA1105_ERROR;
        }
    }

    /* Error occured */
    else if (tx_status != TX_NO_EVENTS) {
        sja_status = SJA1105_ERROR;
    }

    return sja_status;
}
