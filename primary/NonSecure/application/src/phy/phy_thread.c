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
    bool                      link_up     = false;

    UNUSED(cable_state); // TODO: Use

    /* Initialise structs */
    memset((bool *) &phy_temperatures_valid, 0, sizeof(phy_temperatures_valid));

    LOG_INFO("Starting PHY thread");

    /* Initialise PHYs */
    phy_status = phys_init();
    if (phy_status != PHY_OK) error_handler();

    /* Check if any links are up (this calls the corresponding link state change callback which
     * is needed because the link can go up before the interrupt is enabled) */
    for (uint_fast8_t i = 0; i < NUM_PHYS; i++) {
        phy_status = PHY_GetLinkState(phy_handles[i], NULL);
        if (phy_status != PHY_OK) error_handler();
    }

    /* Setup timing control variables (done in ms) */
    uint32_t current_time   = tx_time_get_ms();
    uint32_t next_wake_time = current_time + PHY_THREAD_INTERVAL;

    while (1) {

        /* Sleep until the next wake time while also monitoring for PHY events */
        current_time = tx_time_get_ms();
        if (current_time < next_wake_time) {

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

        /* Read temperatures */
        for (phy_index_t i = 0; i < NUM_PHYS; i++) {
            phy_status = PHY_ReadTemperature(phy_handles[i], &(phy_temperatures[i]), &(phy_temperatures_valid[i]));
            // if (phy_status != PHY_OK) error_handler(); TODO: re-enable when implemented
        }

        /* Poll link states in case an interrupt is missed */
        for (phy_index_t i = 0; i < NUM_PHYS; i++) {
            phy_status = PHY_GetLinkState(phy_handles[i], &link_up);
            if (phy_status != PHY_OK) error_handler();
        }
    }
}


static phy_status_t phy_process_interrupts(uint32_t event_flags) {

    phy_status_t status = PHY_OK;

    /* Call the interrupt handlers */

#if NUM_PHYS > 0
    if (event_flags & PHY_PHY0_EVENT) {
        status = PHY_ProcessInterrupt(&hphy0);
        if (status != PHY_OK) return status;
    }
#endif
#if NUM_PHYS > 1
    if (event_flags & PHY_PHY1_EVENT) {
        status = PHY_ProcessInterrupt(&hphy1);
        if (status != PHY_OK) return status;
    }
#endif
#if NUM_PHYS > 2
    if (event_flags & PHY_PHY2_EVENT) {
        status = PHY_ProcessInterrupt(&hphy2);
        if (status != PHY_OK) return status;
    }
#endif
#if NUM_PHYS > 3
    if (event_flags & PHY_PHY3_EVENT) {
        status = PHY_ProcessInterrupt(&hphy3);
        if (status != PHY_OK) return status;
    }
#endif
#if NUM_PHYS > 4
    if (event_flags & PHY_PHY4_EVENT) {
        status = PHY_ProcessInterrupt(&hphy4);
        if (status != PHY_OK) return status;
    }
#endif
#if NUM_PHYS > 5
    if (event_flags & PHY_PHY5_EVENT) {
        status = PHY_ProcessInterrupt(&hphy5);
        if (status != PHY_OK) return status;
    }
#endif
#if NUM_PHYS > 6
    if (event_flags & PHY_PHY6_EVENT) {
        status = PHY_ProcessInterrupt(&hphy6);
        if (status != PHY_OK) return status;
    }
#endif

    return status;
}


// phy_fault_t fault = PHY_FAULT_NONE;
// phy_status        = PHY_88Q211X_CheckFaults(&hphy0, &fault);
// phy_status         = PHY_88Q211X_CheckFaults(&hphy1, &fault1);
// phy_status         = PHY_88Q211X_CheckFaults(&hphy2, &fault2);
//        if (fault != PHY_FAULT_NONE) {
//            phy_status = PHY_88Q211X_Start100MBIST(&hphy0);
//            if (phy_status != PHY_OK) error_handler();
//            phy_status = PHY_88Q211X_Get100MBISTResults(&hphy0, &error);
//            if (phy_status != PHY_OK) error_handler();
//        }

/* TODO: Move VCT to after a timeout if there is no link (to prioritise startup speed), also the PHY probably needs to be reset after due to magic numbers in undocumented registers */
// /* Start the virtual cable tests (this can take up to 500ms) */
// status = PHY_88Q211X_StartVCT(&hphy0);
// if (status != PHY_OK) error_handler();
// status = PHY_88Q211X_StartVCT(&hphy1);
// if (status != PHY_OK) error_handler();
// status = PHY_88Q211X_StartVCT(&hphy2);
// if (status != PHY_OK) error_handler();

// /* Get the VCT results */
// tx_thread_sleep_ms(500);
// uint32_t                  maximum_peak_distance;
// status = PHY_88Q211X_GetVCTResults(&hphy0, &cable_state, &maximum_peak_distance);
// if (status != PHY_OK) error_handler();
// status = PHY_88Q211X_GetVCTResults(&hphy1, &cable_state, &maximum_peak_distance);
// if (status != PHY_OK) error_handler();
// status = PHY_88Q211X_GetVCTResults(&hphy2, &cable_state, &maximum_peak_distance);
// if (status != PHY_OK) error_handler();


/* TODO: If the current thread holds the phy mutex when it shouldn't report an error */