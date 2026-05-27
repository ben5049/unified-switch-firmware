/*
 * switch_thread.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "logging.h"
#include "stdint.h"
#include "stdatomic.h"

#include "app.h"
#include "tx_app.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "switch_diagnostics.h"
#include "sja1105.h"
#include "utils.h"


uint8_t   switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_handle;

TX_EVENT_FLAGS_GROUP switch_events_handle;

TX_TIMER switch_maintenance_timer;
TX_TIMER switch_publish_timer;

sja1105_stats_detailed_t stats_ext[NUM_SWITCHES];


/* This thread perform regular maintenance for the switch and publishes periodic diagnostic messages */
void switch_thread_entry(uint32_t initial_input) {

    sja1105_status_t status    = SJA1105_OK;
    tx_status_t      tx_status = TX_SUCCESS;
    uint32_t         events;

#if DEBUG
    TX_THREAD *mutex_owner;
#endif

    LOG_INFO("Starting switch thread");

    /* Clear stats structs */
    memset(stats_ext, 0, sizeof(stats_ext));

    status = init_switch_diagnostics();
    if (status != SJA1105_OK) error_handler();

    /* Start the timers */
    tx_status = tx_timer_activate(&switch_maintenance_timer);
    if (tx_status != TX_SUCCESS) error_handler();
    tx_status = tx_timer_activate(&switch_publish_timer);
    if (tx_status != TX_SUCCESS) error_handler();

    while (1) {

        /* Wait for an event from the timers */
        tx_status = tx_event_flags_get(
            &switch_events_handle,
            SWITCH_EVENT_MAINTENANCE | SWITCH_EVENT_PUBLISH,
            TX_OR_CLEAR,
            &events,
            MAX(SWITCH_MAINTENANCE_INTERVAL, SWITCH_PUBLISH_STATS_INTERVAL) * 5 /* Break out of deadlocks */
        );
        if (tx_status != TX_SUCCESS) error_handler();

        /* Check if maintenance is necessary */
        if (events & SWITCH_EVENT_MAINTENANCE) {
            for (switch_index_t i = SWITCH0; i < NUM_SWITCHES; i++) {

                /* Make sure local copies of tables match the copy on the switch chip
                 * (this doesn't check for differences, it only updates the internal copy) */
                status = SJA1105_ReadAllTables(&switch_handles[i]);
                if (status != SJA1105_OK) error_handler();

                /* Check the status registers for issues (including RAM parity errors) */
                status = SJA1105_CheckStatusRegisters(&switch_handles[i]);
                if (status == SJA1105_RAM_PARITY_ERROR) {
                    LOG_WARNING("RAM Parity error detected in switch %d", i);
                    status = switch_reset(i);
                }
                if (status != SJA1105_OK) error_handler();

                /* Read out the stats */
#if SWITCH_GET_EXTENDED_STATS
                status = SJA1105_ReadStatsDetailed(&switch_handles[i], &stats_ext[i]);
                if (status != SJA1105_OK) error_handler();
#else
                status = SJA1105_ReadStatsMAC(&switch_handles[i], &stats_ext[i].mac);
                if (status != SJA1105_OK) error_handler();
#endif

                /* Certain errors can cause memory leaks under exceptional conditions.
                 * If a large number of these errors have occured then reset the switch
                 * (AH1704 section 10.1) */
                for (uint_fast8_t port = 0; port < SJA1105_NUM_PORTS; port++) {

                    if (stats_ext[i].mac.mii_errors[port] > SWITCH_ERR_THRESHOLD ||
                        stats_ext[i].mac.runt_count[port] > SWITCH_ERR_THRESHOLD) {

                        LOG_WARNING("High error count in switch %d", i);

                        status = switch_reset(i);
                        if (status != SJA1105_OK) error_handler();
                    }
                }

                /* Free any management routes that have been used */
                status = SJA1105_ManagementRouteFree(&switch_handles[i], false);
                if (status != SJA1105_OK) error_handler();

                /* TODO: Occasionally check no important MAC addresses have been learned
                 * by accident? (PTP, STP, etc) */

                /* Read the temperature */
                status = SJA1105_ReadTemperature(&switch_handles[i], &switch_info[i].temperature);
                if (status != SJA1105_OK) error_handler();
                switch_info[i].temperature_valid = true;
            }
        }

        /* Check if publishing diagnostics is necessary */
        if (events & SWITCH_EVENT_PUBLISH) {

            /* Attempt to publish the diagnostics */
            status = publish_switch_diagnostics(tx_time_get_ms());
            if (status != SJA1105_OK) error_handler();
        }

        /* If the current thread holds the switch mutex when it shouldn't report an error */
#if DEBUG
        tx_status = tx_mutex_info_get(&switch_mutex_handle, NULL, NULL, &mutex_owner, NULL, NULL, NULL);
        if (tx_status != TX_SUCCESS) error_handler();
        assert(mutex_owner != &switch_thread_handle);
#endif
    }
}
