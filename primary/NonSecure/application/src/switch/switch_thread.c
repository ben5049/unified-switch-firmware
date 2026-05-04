/*
 * switch_thread.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "logging.h"
#include "stdint.h"
#include "stdatomic.h"

#include "main.h"

#include "switch_thread.h"
#include "switch_callbacks.h"
#include "switch_diagnostics.h"
#include "sja1105.h"
#include "utils.h"


uint8_t   switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_handle;

#if SWITCH_GET_EXTENDED_STATS
sja1105_stats_detailed_t stats_ext[NUM_SWITCHES];
#endif


/* This thread perform regular maintenance for the switch and publishes periodic diagnostic messages */
void switch_thread_entry(uint32_t initial_input) {

    sja1105_status_t status;

    uint32_t current_time          = tx_time_get_ms();
    uint32_t next_publish_time     = current_time;
    uint32_t next_maintenance_time = current_time;
    uint32_t next_wakeup           = 0;

    /* Clear stats structs */
#if SWITCH_GET_EXTENDED_STATS
    memset(stats_ext, 0, sizeof(stats_ext));
#endif

    LOG_INFO("Starting switch thread");

    status = init_switch_diagnostics();
    if (status != SJA1105_OK) error_handler();

    while (1) {

        current_time = tx_time_get_ms();

        /* Check if maintenance is necessary */
        if (current_time >= next_maintenance_time) {
            next_maintenance_time += SWITCH_MAINTENANCE_INTERVAL;
            for (switch_index_t i = 0; i < NUM_SWITCHES; i++) {

                /* Make sure local copies of tables match the copy on the switch chip (this doesn't check for differences, it only updates the internal copy) */
                status = SJA1105_ReadAllTables(&switch_handles[i]);
                if (status != SJA1105_OK) error_handler();

                /* Check the status registers for issues */
                status = SJA1105_CheckStatusRegisters(&switch_handles[i]);
                if (status != SJA1105_OK) error_handler();

                /* Read out the detailed stats */
#if SWITCH_GET_EXTENDED_STATS
                status = SJA1105_ReadStatsDetailed(&switch_handles[i], &stats_ext[i]);
                if (status != SJA1105_OK) error_handler();
#endif

                /* Free any management routes that have been used */
                status = SJA1105_ManagementRouteFree(&switch_handles[i], false);
                if (status != SJA1105_OK) error_handler();

                /* TODO: Occasionally check no important MAC addresses have been learned by accident? (PTP, STP, etc) */

                /* Read the temperature */
                status = SJA1105_ReadTemperature(&switch_handles[i], &switch_info[i].temperature);
                if (status != SJA1105_OK) error_handler();
                switch_info[i].temperature_valid = true;
            }
        }

        /* Check if publishing diagnostics is necessary */
        if (current_time >= next_publish_time) {
            next_publish_time += SWITCH_PUBLISH_STATS_INTERVAL;

            /* Attempt to publish the diagnostics */
            status = publish_switch_diagnostics(current_time);
            if (status != SJA1105_OK) error_handler();
        }

        /* Schedule the next wakeup */
        next_wakeup = MIN(next_maintenance_time, next_publish_time);
        if (current_time < next_wakeup) {
            tx_thread_sleep_ms(next_wakeup - current_time);
        }

        /* Somehow we have gotten far behind so catch up */
        else if ((current_time - next_wakeup) > (MAX(SWITCH_MAINTENANCE_INTERVAL, SWITCH_PUBLISH_STATS_INTERVAL) * 3)) {
            next_maintenance_time = current_time;
            next_publish_time     = current_time;
        }

        /* TODO: If the current thread holds the switch mutex when it shouldn't report an error */
    }
}
