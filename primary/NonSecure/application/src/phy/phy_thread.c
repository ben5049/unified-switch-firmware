/*
 * phy_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "hal.h"
#include "main.h"

#include "88q211x.h"
#include "lan867x.h"
#include "phy_thread.h"
#include "phy_callbacks.h"
#include "utils.h"
#include "config.h"
#include "tx_app.h"


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

    memset((bool *) &phy_temperatures_valid, 0, sizeof(phy_temperatures_valid));

    /* Initialise PHYs */
    phy_status = phys_init();
    if (phy_status != PHY_OK) Error_Handler();

    /* Check if any links are up (also call the corresponding callback which is needed because the link can go up before the interrupt is enabled) */
    phy_status = PHY_88Q211X_GetLinkState(&hphy1, &link_up);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_GetLinkState(&hphy2, &link_up);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_GetLinkState(&hphy3, &link_up);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_GetLinkState(&hphy4, &link_up);
    if (phy_status != PHY_OK) Error_Handler();
    phy_status = PHY_88Q211X_GetLinkState(&hphy5, &link_up);
    if (phy_status != PHY_OK) Error_Handler();

    /* Setup timing control variables (done in ms) */
    uint32_t current_time   = tx_time_get_ms();
    uint32_t next_wake_time = current_time + PHY_THREAD_INTERVAL;

    while (1) {

        /* Sleep until the next wake time while also monitoring for PHY events */
        current_time = tx_time_get_ms();
        if (current_time < next_wake_time) {

            /* Wait for an interrupt */
            tx_status = tx_event_flags_get(&phy_events_handle, PHY_ALL_EVENTS, TX_OR_CLEAR, &event_flags, MS_TO_TICKS(next_wake_time - current_time));
            if ((tx_status != TX_SUCCESS) && (tx_status != TX_NO_EVENTS)) Error_Handler();

            /* Process any interrupts */
            if (event_flags && (tx_status != TX_NO_EVENTS)) {

                /* Call the interrupt handlers */
                if (event_flags & PHY_PHY0_EVENT) {
                    phy_status = PHY_ProcessInterrupt(&hphy0);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                if (event_flags & PHY_PHY1_EVENT) {
                    phy_status = PHY_ProcessInterrupt(&hphy1);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                if (event_flags & PHY_PHY2_EVENT) {
                    phy_status = PHY_ProcessInterrupt(&hphy2);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                if (event_flags & PHY_PHY3_EVENT) {
                    phy_status = PHY_ProcessInterrupt(&hphy3);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                if (event_flags & PHY_PHY4_EVENT) {
                    phy_status = PHY_ProcessInterrupt(&hphy4);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                if (event_flags & PHY_PHY5_EVENT) {
                    phy_status = PHY_ProcessInterrupt(&hphy5);
                    if (phy_status != PHY_OK) Error_Handler();
                }
                if (event_flags & PHY_PHY6_EVENT) {
                    phy_status = PHY_ProcessInterrupt(&hphy6);
                    if (phy_status != PHY_OK) Error_Handler();
                }

                /* Go to the start of the loop and go back to sleep if necessary */
                continue;
            }

            /* Otherwise the sleep time is over and regular PHY processing must be done */
        }

        /* Schedule next wake time */
        next_wake_time += PHY_THREAD_INTERVAL;

        /* Read temperatures */
        // phy_status = PHY_88Q211X_ReadTemperature(&hphy0, &(phy_temperatures[0]), &(phy_temperatures_valid[0]));
        // if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_ReadTemperature(&hphy1, &(phy_temperatures[1]), &(phy_temperatures_valid[1]));
        if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_ReadTemperature(&hphy2, &(phy_temperatures[2]), &(phy_temperatures_valid[2]));
        if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_ReadTemperature(&hphy3, &(phy_temperatures[3]), &(phy_temperatures_valid[3]));
        if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_ReadTemperature(&hphy4, &(phy_temperatures[4]), &(phy_temperatures_valid[4]));
        if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_ReadTemperature(&hphy5, &(phy_temperatures[5]), &(phy_temperatures_valid[5]));
        if (phy_status != PHY_OK) Error_Handler();

        /* Poll link states in case an interrupt is missed */
        // phy_status = PHY_88Q211X_GetLinkState(&hphy0, &link_up);
        // if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_GetLinkState(&hphy1, &link_up);
        if (phy_status != PHY_OK) Error_Handler();
        phy_status = PHY_88Q211X_GetLinkState(&hphy2, &link_up);
        if (phy_status != PHY_OK) Error_Handler();

        // phy_fault_t fault = PHY_FAULT_NONE;
        // phy_status        = PHY_88Q211X_CheckFaults(&hphy0, &fault);
        // phy_status         = PHY_88Q211X_CheckFaults(&hphy1, &fault1);
        // phy_status         = PHY_88Q211X_CheckFaults(&hphy2, &fault2);
        //        if (fault != PHY_FAULT_NONE) {
        //            phy_status = PHY_88Q211X_Start100MBIST(&hphy0);
        //            if (phy_status != PHY_OK) Error_Handler();
        //            phy_status = PHY_88Q211X_Get100MBISTResults(&hphy0, &error);
        //            if (phy_status != PHY_OK) Error_Handler();
        //        }

        /* TODO: Move VCT to after a timeout if there is no link (to prioritise startup speed), also the PHY probably needs to be reset after due to magic numbers in undocumented registers */
        // /* Start the virtual cable tests (this can take up to 500ms) */
        // status = PHY_88Q211X_StartVCT(&hphy0);
        // if (status != PHY_OK) Error_Handler();
        // status = PHY_88Q211X_StartVCT(&hphy1);
        // if (status != PHY_OK) Error_Handler();
        // status = PHY_88Q211X_StartVCT(&hphy2);
        // if (status != PHY_OK) Error_Handler();

        // /* Get the VCT results */
        // tx_thread_sleep_ms(500);
        // uint32_t                  maximum_peak_distance;
        // status = PHY_88Q211X_GetVCTResults(&hphy0, &cable_state, &maximum_peak_distance);
        // if (status != PHY_OK) Error_Handler();
        // status = PHY_88Q211X_GetVCTResults(&hphy1, &cable_state, &maximum_peak_distance);
        // if (status != PHY_OK) Error_Handler();
        // status = PHY_88Q211X_GetVCTResults(&hphy2, &cable_state, &maximum_peak_distance);
        // if (status != PHY_OK) Error_Handler();


        /* TODO: If the current thread holds the phy mutex when it shouldn't report an error */
    }
}
