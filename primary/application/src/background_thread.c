/*
 * background_thread.c
 *
 *  Created on: Oct 5, 2025
 *      Author: bens1
 */

#include "secure_nsc.h"
#include "stdint.h"

#include "background_thread.h"
#include "utils.h"
#include "metadata.h"
#include "error.h"
#include "secure_nsc.h"
#include "config.h"


TX_THREAD background_thread_handle;
uint8_t   background_thread_stack[BACKGROUND_THREAD_STACK_SIZE];


void background_thread_entry(uint32_t initial_input) {

    uint32_t current_time = tx_time_get_ms();
    uint32_t next_wakeup  = current_time + BACKGROUND_THREAD_INTERVAL;

    while (1) {

        current_time = tx_time_get_ms();

        /* Do background tasks in the secure world
         * - TODO: Read all flash and RAM for ECC errors
         * - Check if changes have been made to the metadata and sync them to the FRAM
         */
        s_background_task();

        /* Schedule the next wakeup */
        next_wakeup += BACKGROUND_THREAD_INTERVAL;
        if (current_time < next_wakeup) {
            tx_thread_sleep_ms(next_wakeup - current_time);
        }

        /* Somehow we have gotten far behind so catch up */
        else if ((current_time - next_wakeup) > (BACKGROUND_THREAD_INTERVAL * 3)) {
            next_wakeup = current_time;
        }
    }
}
