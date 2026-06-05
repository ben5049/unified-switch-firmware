/*
 * background_thread.c
 *
 *  Created on: Oct 5, 2025
 *      Author: bens1
 */

#include "secure_nsc.h"
#include "stdint.h"

#include "dts.h"

#include "tx_app.h"
#include "nx_app.h"
#include "background_thread.h"
#include "utils.h"
#include "secure_nsc.h"


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

    hal_status_t hal_status = HAL_OK;
    tx_status_t  tx_status  = TX_SUCCESS;
    hal_status_t nx_status  = NX_SUCCESS;
    uint32_t     events;

    int32_t temperature;

    uint32_t total_packets;
    uint32_t free_packets;

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

        /* Check packet pool free counts */
        nx_status = nx_packet_pool_info_get(&nx_small_packet_pool, &total_packets, &free_packets, NULL, NULL, NULL);
        NX_CHECK(nx_status);
        if (free_packets <= ((total_packets * 3) / 10)) LOG_WARNING("Warning small packet pool low free count: %lu/%lu", free_packets, total_packets);
        nx_status = nx_packet_pool_info_get(&nx_big_packet_pool, &total_packets, &free_packets, NULL, NULL, NULL);
        NX_CHECK(nx_status);
        if (free_packets <= ((total_packets * 3) / 10)) LOG_WARNING("Warning big packet pool low free count: %lu/%lu", free_packets, total_packets);

        /* Do background tasks in the secure world
         * - TODO: Read all flash and RAM for ECC errors
         * - Check if changes have been made to the metadata and sync them to the FRAM
         */
        s_background_task();
    }
}
