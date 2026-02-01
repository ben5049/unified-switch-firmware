/*
 * tx_app.c
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 */

#include "tx_app.h"
#include "app_setup.h"
#include "main.h"
#include "nx_app.h"
#include "nx_link_thread.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "phy_thread.h"
#include "phy_callbacks.h"
#include "stp_thread.h"
#include "comms_thread.h"
#include "ptp_thread.h"
#include "state_machine.h"
#include "background_thread.h"
#include "config.h"
#include "utils.h"


/* TODO: Check all function return values */

/* This function should be called once in App_ThreadX_Init */
// clang-format off
void tx_setup(void *memory_ptr) {

    uint8_t thread_number = 1;

    /* Create mutexes */
    tx_mutex_create(&sja1105_mutex_handle, "sja1105_mutex", TX_INHERIT);
    tx_mutex_create(&phy_mutex_handle,     "phy_mutex",     TX_INHERIT);

    /* Create semaphores */

    /* Create event flags */
    tx_event_flags_create(&state_machine_events_handle, "state_machine_events_handle");
//    tx_event_flags_create(&stp_events_handle,           "stp_events_handle");
    tx_event_flags_create(&phy_events_handle,           "phy_events_handle");

    /* Create queues */
    tx_queue_create(&ptp_tx_queue_handle, "ptp_tx_queue", sizeof(nx_ptp_tx_info_t), ptp_tx_queue_stack, PTP_TX_QUEUE_SIZE);

    /* Create threads */
    tx_thread_create(&state_machine_thread_handle, "state_machine_thread", (void (*)(ULONG)) state_machine_thread_entry, thread_number++, state_machine_thread_stack, STATE_MACHINE_THREAD_STACK_SIZE, STATE_MACHINE_THREAD_PRIORITY, STATE_MACHINE_THREAD_PRIORITY,    TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_thread_create(&nx_link_thread_handle,       "nx_link_thread",       (void (*)(ULONG)) nx_link_thread_entry,       thread_number++, nx_link_thread_stack,       NX_LINK_THREAD_STACK_SIZE,       NX_LINK_THREAD_PRIORITY,       NX_LINK_THREAD_PRIORITY,          TX_NO_TIME_SLICE, TX_DONT_START);
    tx_thread_create(&switch_thread_handle,        "switch_thread",        (void (*)(ULONG)) switch_thread_entry,        thread_number++, switch_thread_stack,        SWITCH_THREAD_STACK_SIZE,        SWITCH_THREAD_PRIORITY,        SWITCH_THREAD_PREMPTION_PRIORITY, 1,                TX_DONT_START);
    tx_thread_create(&phy_thread_handle,           "phy_thread",           (void (*)(ULONG)) phy_thread_entry,           thread_number++, phy_thread_stack,           PHY_THREAD_STACK_SIZE,           PHY_THREAD_PRIORITY,           PHY_THREAD_PREMPTION_PRIORITY,    1,                TX_DONT_START);
    // tx_thread_create(&stp_thread_handle,           "stp_thread",           (void (*)(ULONG)) stp_thread_entry,           thread_number++, stp_thread_stack,           STP_THREAD_STACK_SIZE,           STP_THREAD_PRIORITY,           STP_THREAD_PREMPTION_PRIORITY,    1,                TX_DONT_START);
    tx_thread_create(&comms_thread_handle,         "comms_thread",         (void (*)(ULONG)) comms_thread_entry,         thread_number++, comms_thread_stack,         COMMS_THREAD_STACK_SIZE,         COMMS_THREAD_PRIORITY,         COMMS_THREAD_PREMPTION_PRIORITY,  1,                TX_DONT_START);
    tx_thread_create(&ptp_thread_handle,           "ptp_thread",           (void (*)(ULONG)) ptp_thread_entry,           thread_number++, ptp_thread_stack,           PTP_THREAD_STACK_SIZE,           PTP_THREAD_PRIORITY,           PTP_THREAD_PRIORITY,              TX_NO_TIME_SLICE, TX_DONT_START);
    tx_thread_create(&background_thread_handle,    "background_thread",    (void (*)(ULONG)) background_thread_entry,    thread_number++, background_thread_stack,    BACKGROUND_THREAD_STACK_SIZE,    BACKGROUND_THREAD_PRIORITY,    BACKGROUND_THREAD_PRIORITY,       TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Any threads that call secure functions must allocate secure stack */
    tx_thread_secure_stack_allocate(&state_machine_thread_handle, MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    tx_thread_secure_stack_allocate(&nx_link_thread_handle,       MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    tx_thread_secure_stack_allocate(&switch_thread_handle,        MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    tx_thread_secure_stack_allocate(&phy_thread_handle,           MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    // tx_thread_secure_stack_allocate(&stp_thread_handle,           MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    tx_thread_secure_stack_allocate(&comms_thread_handle,         MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    tx_thread_secure_stack_allocate(&ptp_thread_handle,           MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    tx_thread_secure_stack_allocate(&background_thread_handle,    MIN(BACKGROUND_THREAD_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM)); /* More stack required for secure background tasks */

    /* Assign loggers to threads */
    state_machine_thread_handle.logger = &hlog_generic;
    nx_link_thread_handle.logger       = &hlog_network;
    switch_thread_handle.logger        = &hlog_sw;
    phy_thread_handle.logger           = &hlog_phy;
    // stp_thread_handle.logger           = &hlog_network;
    comms_thread_handle.logger         = &hlog_comms;
    ptp_thread_handle.logger           = &hlog_network;
    background_thread_handle.logger    = &hlog_generic;
}
// clang-format on
