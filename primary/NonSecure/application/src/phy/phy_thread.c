/*
 * phy_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "hal.h"

#include "app.h"
#include "88q211x.h"
#include "lan867x.h"
#include "phy_thread.h"
#include "phy_callbacks.h"
#include "utils.h"
#include "tx_app.h"


/* Private function prototypes */
static phy_status_t phy_process_interrupts(uint32_t event_flags);


uint8_t   phy_thread_stack[PHY_THREAD_STACK_SIZE];
TX_THREAD phy_thread_handle;

float phy_temperatures[NUM_PHYS];
bool  phy_temperatures_valid[NUM_PHYS];


void phy_thread_entry(uint32_t initial_input) {

    phy_status_t              phy_status  = PHY_OK;
    tx_status_t               tx_status   = TX_SUCCESS;
    phy_cable_state_88q211x_t cable_state = PHY_CABLE_STATE_88Q211X_NOT_STARTED;
    uint32_t                  event_flags = 0;

    UNUSED(cable_state); // TODO: Use

    /* Initialise structs */
    memset((bool *) &phy_temperatures_valid, 0, sizeof(phy_temperatures_valid));

    LOG_INFO("Starting PHY thread");

    /* Initialise PHYs */
    phy_status = phys_init();
    if (phy_status != PHY_OK) error_handler();

    /* Setup timing control variables (done in ms) */
    uint32_t current_time   = tx_time_get_ms();
    uint32_t next_wake_time = current_time + PHY_THREAD_INTERVAL;
    uint32_t last_temp_read = current_time;

    while (1) {

        /* Sleep until the next wake time while also monitoring for PHY events */
        current_time = tx_time_get_ms();
        if ((int32_t) (current_time - next_wake_time) < 0) {

            /* Wait for an interrupt */
            tx_status = tx_event_flags_get(&phy_events_handle, PHY_ALL_EVENTS, TX_OR_CLEAR, (ULONG *) &event_flags, MS_TO_TICKS(next_wake_time - current_time));
            if ((tx_status != TX_SUCCESS) && (tx_status != TX_NO_EVENTS)) error_handler();

            /* Process any interrupts */
            if (event_flags && (tx_status != TX_NO_EVENTS)) {
                phy_status = phy_process_interrupts(event_flags);
                if (phy_status != PHY_OK) error_handler();

                /* Go to the start of the loop and go back to sleep if necessary */
                continue;
            }

            /* Otherwise the sleep time is over and regular PHY processing must be done */
        }

        /* Schedule next wake time */
        next_wake_time += PHY_THREAD_INTERVAL;

        /* Update PHY state machines */
        for (phy_index_t i = 0; i < NUM_PHYS; i++) {
            phy_status = phy_state_update_poll((phy_handle_base_t *) phy_handles[i], current_time);
            if (phy_status != PHY_OK) error_handler();
        }

        /* Read temperatures */
        if ((current_time - last_temp_read) >= PHY_TEMPERATURE_READ_INTERVAL) {
            for (phy_index_t i = 0; i < NUM_PHYS; i++) {
                phy_status = PHY_ReadTemperature(phy_handles[i], &(phy_temperatures[i]), &(phy_temperatures_valid[i]));
                // if (phy_status != PHY_OK) error_handler(); TODO: re-enable when implemented
            }
            last_temp_read = current_time;
        }
    }
}


static phy_status_t phy_process_interrupts(uint32_t event_flags) {

    phy_status_t status = PHY_OK;

    /* Call the interrupt handlers */
    for (uint_fast8_t i = 0; i < NUM_PHYS; i++) {
        if (event_flags & (PHY_PHY0_EVENT << i)) {
            status = PHY_ProcessInterrupt(phy_handles[i]);
            if (status != PHY_OK) return status;
            status = phy_state_update_interrupt((phy_handle_base_t *) phy_handles[i], tx_time_get_ms());
            if (status != PHY_OK) return status;
        }
    }

    return status;
}
