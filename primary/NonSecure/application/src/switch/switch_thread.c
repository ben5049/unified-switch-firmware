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

#if HW_VERSION == 4
#include "swv4_sja1105_static_config_default.h"
#elif HW_VERSION == 5
#include "swv5_0_sja1105_static_config_default.h"
#include "swv5_1_sja1105_static_config_default.h"
#endif


uint8_t   switch_thread_stack[SWITCH_THREAD_STACK_SIZE];
TX_THREAD switch_thread_handle;

atomic_uint_fast32_t sja1105_error_counter = 0;
float                switch_temperatures[NUM_SWITCHES];
bool                 switch_temperatures_valid[NUM_SWITCHES];


/* This thread perform regular maintenance for the switch and publishes periodic diagnostic messages */
void switch_thread_entry(uint32_t initial_input) {

    sja1105_status_t status;

    uint32_t current_time          = tx_time_get_ms();
    uint32_t next_publish_time     = current_time;
    uint32_t next_maintenance_time = current_time;
    uint32_t next_wakeup           = 0;

    /* Clear temperature variables */
    for (uint_fast8_t i = 0; i < NUM_SWITCHES; i++) {
        switch_temperatures[i]       = 0.0;
        switch_temperatures_valid[i] = false;
    }

    LOG_INFO("Starting switch thread");

    status = init_switch_diagnostics();
    if (status != SJA1105_OK) error_handler();

    while (1) {

        current_time = tx_time_get_ms();

        /* Check if maintenance is necessary */
        if (current_time >= next_maintenance_time) {
            next_maintenance_time += SWITCH_MAINTENANCE_INTERVAL;

            /* Make sure local copies of tables match the copy on the switch chip (this doesn't check for differences, it only updates the internal copy) */
            status = SJA1105_ReadAllTables(&hsw0);
            if (status != SJA1105_OK) error_handler();
#if HW_VERSION == 5
            status = SJA1105_ReadAllTables(&hsw1);
            if (status != SJA1105_OK) error_handler();
#endif

            /* Check the status registers for issues */
            status = SJA1105_CheckStatusRegisters(&hsw0); // TODO: look into buffer shifting issue
            if (status != SJA1105_OK) error_handler();
#if HW_VERSION == 5
            status = SJA1105_CheckStatusRegisters(&hsw1);
            if (status != SJA1105_OK) error_handler();
#endif

            sja1105_statistics_t stats0, stats1;
            status = SJA1105_ReadStatistics(&hsw0, &stats0);
            if (status != SJA1105_OK) error_handler();
            status = SJA1105_ReadStatistics(&hsw1, &stats1);
            if (status != SJA1105_OK) error_handler();

            /* Free any management routes that have been used */
            status = SJA1105_ManagementRouteFree(&hsw0, false);
            if (status != SJA1105_OK) error_handler();
#if HW_VERSION == 5
            status = SJA1105_ManagementRouteFree(&hsw1, false);
            if (status != SJA1105_OK) error_handler();
#endif

            /* TODO: Occasionally check no important MAC addresses have been learned by accident (PTP, STP, etc) */

            /* Read the temperature */
            status = SJA1105_ReadTemperature(&hsw0, &(switch_temperatures[0]));
            if (status != SJA1105_OK) error_handler();
            switch_temperatures_valid[0] = true;
#if HW_VERSION == 5
            status = SJA1105_ReadTemperature(&hsw1, &(switch_temperatures[1]));
            if (status != SJA1105_OK) error_handler();
            switch_temperatures_valid[1] = true;
#endif
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
