/*
 * tx_app.c
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 */

#include "app.h"
#include "tx_app.h"
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
#include "utils.h"


/* This function should be called once in App_ThreadX_Init */
// clang-format off
void tx_setup(void *memory_ptr) {

    tx_status_t status        = TX_SUCCESS;
    uint8_t     thread_number = 1;

    /* Enable tracex */
#if TRACE_ENABLE
    status = tx_trace_enable(TRACE_BUFFER_START, TRACE_BUFFER_SIZE, TRACE_REGISTRY_ENTRIES);
    if (status != TX_SUCCESS) error_handler();
#endif


#ifdef TX_ENABLE_STACK_CHECKING
    status = tx_thread_stack_error_notify(thread_stack_error_handler);
    if (status != TX_SUCCESS) error_handler();
#endif

    /* Create mutexes */
    status = tx_mutex_create(&switch_mutex_handle,  "switch_mutex",  TX_INHERIT);
    if (status != TX_SUCCESS) error_handler();
    status = tx_mutex_create(&phy_mutex_handle,     "phy_mutex",     TX_INHERIT);
    if (status != TX_SUCCESS) error_handler();

    /* Create semaphores */

    /* Create event flags */
    status = tx_event_flags_create(&state_machine_events_handle, "state_machine_events_handle");
    if (status != TX_SUCCESS) error_handler();
    status = tx_event_flags_create(&switch_events_handle,        "switch_events_handle");
    if (status != TX_SUCCESS) error_handler();
#if ENABLE_STP_THREAD
    status = tx_event_flags_create(&stp_events_handle,           "stp_events_handle");
    if (status != TX_SUCCESS) error_handler();
#endif
    status = tx_event_flags_create(&phy_events_handle,           "phy_events_handle");
    if (status != TX_SUCCESS) error_handler();
#if ENABLE_PTP_THREAD
    status = tx_event_flags_create(&ptp_tx_events_handle,        "ptp_tx_events_handle");
    if (status != TX_SUCCESS) error_handler();
    status = tx_event_flags_create(&ptp_clock_events_handle,     "ptp_clock_events_handle");
    if (status != TX_SUCCESS) error_handler();
#endif

    /* Create queues */
#if ENABLE_PTP_THREAD
    status = tx_queue_create(&ptp_event_queue_handle,     "ptp_event_queue",     PTP_CLIENT_MSG_SIZE_WORDS, ptp_event_queue_stack,     sizeof(ptp_event_queue_stack));
    if (status != TX_SUCCESS) error_handler();
    status = tx_queue_create(&ptp_tx_queue_handle,        "ptp_tx_queue",        PTP_PACKET_MSG_SIZE_WORDS, ptp_tx_queue_stack,        sizeof(ptp_tx_queue_stack));
    if (status != TX_SUCCESS) error_handler();
    status = tx_queue_create(&ptp_rx_packet_queue_handle, "ptp_rx_packet_queue", PTP_PACKET_MSG_SIZE_WORDS, ptp_rx_packet_queue_stack, sizeof(ptp_rx_packet_queue_stack));
    if (status != TX_SUCCESS) error_handler();
    status = tx_queue_create(&ptp_rx_meta_queue_handle,   "ptp_rx_meta_queue",   PTP_PACKET_MSG_SIZE_WORDS, ptp_rx_meta_queue_stack,   sizeof(ptp_rx_meta_queue_stack));
    if (status != TX_SUCCESS) error_handler();
    status = tx_queue_create(&ptp_clock_queue_handle,     "ptp_clock_queue",     PTP_PACKET_MSG_SIZE_WORDS, ptp_clock_queue_stack,     sizeof(ptp_clock_queue_stack));
    if (status != TX_SUCCESS) error_handler();
#endif

    /* Create threads */
    status = tx_thread_create(&state_machine_thread_handle, "state_machine_thread", (void (*)(ULONG)) state_machine_thread_entry, thread_number++, state_machine_thread_stack, STATE_MACHINE_THREAD_STACK_SIZE, STATE_MACHINE_THREAD_PRIORITY, STATE_MACHINE_THREAD_PRIORITY,    TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_create(&nx_link_thread_handle,       "nx_link_thread",       (void (*)(ULONG)) nx_link_thread_entry,       thread_number++, nx_link_thread_stack,       NX_LINK_THREAD_STACK_SIZE,       NX_LINK_THREAD_PRIORITY,       NX_LINK_THREAD_PRIORITY,          TX_NO_TIME_SLICE, TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_create(&switch_thread_handle,        "switch_thread",        (void (*)(ULONG)) switch_thread_entry,        thread_number++, switch_thread_stack,        SWITCH_THREAD_STACK_SIZE,        SWITCH_THREAD_PRIORITY,        SWITCH_THREAD_PREMPTION_PRIORITY, 1,                TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_create(&phy_thread_handle,           "phy_thread",           (void (*)(ULONG)) phy_thread_entry,           thread_number++, phy_thread_stack,           PHY_THREAD_STACK_SIZE,           PHY_THREAD_PRIORITY,           PHY_THREAD_PREMPTION_PRIORITY,    1,                TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
#if ENABLE_STP_THREAD
    status = tx_thread_create(&stp_thread_handle,           "stp_thread",           (void (*)(ULONG)) stp_thread_entry,           thread_number++, stp_thread_stack,           STP_THREAD_STACK_SIZE,           STP_THREAD_PRIORITY,           STP_THREAD_PREMPTION_PRIORITY,    1,                TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
#endif
#if ENABLE_COMMS_THREAD
    status = tx_thread_create(&comms_thread_handle,         "comms_thread",         (void (*)(ULONG)) comms_thread_entry,         thread_number++, comms_thread_stack,         COMMS_THREAD_STACK_SIZE,         COMMS_THREAD_PRIORITY,         COMMS_THREAD_PREMPTION_PRIORITY,  1,                TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
#endif
#if ENABLE_PTP_THREAD
    status = tx_thread_create(&ptp_event_thread_handle,     "ptp_event_thread",     (void (*)(ULONG)) ptp_event_thread_entry,     thread_number++, ptp_event_thread_stack,     PTP_EVENT_THREAD_STACK_SIZE,     PTP_EVENT_THREAD_PRIORITY,     PTP_EVENT_THREAD_PRIORITY,        TX_NO_TIME_SLICE, TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_create(&ptp_tx_thread_handle,        "ptp_tx_thread",        (void (*)(ULONG)) ptp_tx_thread_entry,        thread_number++, ptp_tx_thread_stack,        PTP_TX_THREAD_STACK_SIZE,        PTP_TX_THREAD_PRIORITY,        PTP_TX_THREAD_PRIORITY,           TX_NO_TIME_SLICE, TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_create(&ptp_rx_thread_handle,        "ptp_rx_thread",        (void (*)(ULONG)) ptp_rx_thread_entry,        thread_number++, ptp_rx_thread_stack,        PTP_RX_THREAD_STACK_SIZE,        PTP_RX_THREAD_PRIORITY,        PTP_RX_THREAD_PRIORITY,           TX_NO_TIME_SLICE, TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_create(&ptp_clock_thread_handle,     "ptp_clock_thread",     (void (*)(ULONG)) ptp_clock_thread_entry,     thread_number++, ptp_clock_thread_stack,     PTP_CLOCK_THREAD_STACK_SIZE,     PTP_CLOCK_THREAD_PRIORITY,     PTP_CLOCK_THREAD_PRIORITY,        TX_NO_TIME_SLICE, TX_DONT_START);
    if (status != TX_SUCCESS) error_handler();
#endif
    status = tx_thread_create(&background_thread_handle,    "background_thread",    (void (*)(ULONG)) background_thread_entry,    thread_number++, background_thread_stack,    BACKGROUND_THREAD_STACK_SIZE,    BACKGROUND_THREAD_PRIORITY,    BACKGROUND_THREAD_PRIORITY,       TX_NO_TIME_SLICE, TX_AUTO_START);
    if (status != TX_SUCCESS) error_handler();

    /* Any threads that call secure functions must allocate secure stack */
    status = tx_thread_secure_stack_allocate(&state_machine_thread_handle, MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_secure_stack_allocate(&nx_link_thread_handle,       MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_secure_stack_allocate(&switch_thread_handle,        MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_secure_stack_allocate(&phy_thread_handle,           MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
#if ENABLE_STP_THREAD
    status = tx_thread_secure_stack_allocate(&stp_thread_handle,           MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
#endif
#if ENABLE_COMMS_THREAD
    status = tx_thread_secure_stack_allocate(&comms_thread_handle,         MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
#endif
#if ENABLE_PTP_THREAD
    status = tx_thread_secure_stack_allocate(&ptp_event_thread_handle,     MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_secure_stack_allocate(&ptp_tx_thread_handle,        MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_secure_stack_allocate(&ptp_rx_thread_handle,        MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
    status = tx_thread_secure_stack_allocate(&ptp_clock_thread_handle,     MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    if (status != TX_SUCCESS) error_handler();
#endif
    status = tx_thread_secure_stack_allocate(&background_thread_handle,    MIN(BACKGROUND_THREAD_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM)); /* More stack required for secure background tasks */
    if (status != TX_SUCCESS) error_handler();

    /* Assign loggers to threads */
    state_machine_thread_handle.logger = &hlog_generic;
    nx_link_thread_handle.logger       = &hlog_network;
    switch_thread_handle.logger        = &hlog_sw;
    phy_thread_handle.logger           = &hlog_phy;
    stp_thread_handle.logger           = &hlog_network;
    comms_thread_handle.logger         = &hlog_comms;
    ptp_event_thread_handle.logger     = &hlog_network;
    ptp_tx_thread_handle.logger        = &hlog_network;
    ptp_rx_thread_handle.logger        = &hlog_network;
    ptp_clock_thread_handle.logger     = &hlog_network;
    background_thread_handle.logger    = &hlog_generic;

    /* Create timers */
    status = tx_timer_create(&switch_maintenance_timer, "switch_maintenance_timer", switch_maintenance_timer_callback, 0, SWITCH_MAINTENANCE_INTERVAL,   SWITCH_MAINTENANCE_INTERVAL,   TX_NO_ACTIVATE);
    if (status != TX_SUCCESS) error_handler();
    status = tx_timer_create(&switch_publish_timer,     "switch_publish_timer",     switch_publish_timer_callback,     0, SWITCH_PUBLISH_STATS_INTERVAL, SWITCH_PUBLISH_STATS_INTERVAL, TX_NO_ACTIVATE);
    if (status != TX_SUCCESS) error_handler();
#if ENABLE_PTP_THREAD
    status = tx_timer_create(&ptp_mac_sync_timer,       "ptp_mac_sync_timer",       ptp_mac_sync_timer_callback,       0, PTP_MAC_SYNC_INTERVAL,         PTP_MAC_SYNC_INTERVAL,         TX_NO_ACTIVATE);
    if (status != TX_SUCCESS) error_handler();
#if NUM_SWITCHES > 1
    status = tx_timer_create(&ptp_switch_sync_timer,    "ptp_switch_sync_timer",    ptp_switch_sync_timer_callback,    0, PTP_SWITCH_SYNC_INTERVAL,       PTP_SWITCH_SYNC_INTERVAL,       TX_NO_ACTIVATE);
    if (status != TX_SUCCESS) error_handler();
#endif
#endif


}

// clang-format on
