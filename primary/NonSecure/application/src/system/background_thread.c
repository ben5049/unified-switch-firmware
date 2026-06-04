/*
 * background_thread.c
 *
 *  Created on: Oct 5, 2025
 *      Author: bens1
 */

#include "secure_nsc.h"
#include "stdint.h"

#include "dts.h"

#include "background_thread.h"
#include "utils.h"
#include "secure_nsc.h"
#include "config.h"


#define BACKGROUND_THREAD_READY (1)


TX_THREAD background_thread_handle;
uint8_t   background_thread_stack[BACKGROUND_THREAD_STACK_SIZE];

TX_EVENT_FLAGS_GROUP background_thread_events_handle;

TX_TIMER background_thread_timer;


void background_thread_timer_callback(ULONG id) {
    tx_status_t status = tx_event_flags_set(&background_thread_events_handle, BACKGROUND_THREAD_READY, TX_OR);
    TX_CHECK(status);
}


void background_thread_entry(uint32_t initial_input) {

    tx_status_t  tx_status  = TX_SUCCESS;
    hal_status_t hal_status = HAL_OK;
    uint32_t     events;
    int32_t      temperature;

    /* Start the digital temperature sensor */
    hal_status = HAL_DTS_Start(&hdts);
    HAL_CHECK(hal_status);

    /* Start the timer */
    tx_status = tx_timer_activate(&background_thread_timer);
    TX_CHECK(tx_status);

    while (1) {

        /* Wait for an event from the timer */
        tx_status = tx_event_flags_get(
            &background_thread_events_handle,
            BACKGROUND_THREAD_READY,
            TX_OR_CLEAR,
            &events,
            TX_WAIT_FOREVER);
        TX_CHECK(tx_status);

        /* Get the temperature */
        hal_status = HAL_DTS_GetTemperature(&hdts, &temperature);
        HAL_CHECK(hal_status);
        if (temperature > 65) {
            LOG_WARNING("Warning high MCU temperature: %li C", temperature);
        }

        /* Do background tasks in the secure world
         * - TODO: Read all flash and RAM for ECC errors
         * - Check if changes have been made to the metadata and sync them to the FRAM
         */
        s_background_task();
    }
}
