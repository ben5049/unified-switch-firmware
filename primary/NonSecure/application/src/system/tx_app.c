/*
 * tx_app.c
 *
 *  Created on: Jul 27, 2025
 *      Author: bens1
 */

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "switch.h"
#include "phy.h"
#include "stp_thread.h"
#include "comms_thread.h"
#include "ptp.h"
#include "sequencer.h"
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
    TX_CHECK(status);
#endif


#ifdef TX_ENABLE_STACK_CHECKING
    status = tx_thread_stack_error_notify(thread_stack_error_handler);
    TX_CHECK(status);
#endif

    /* Create mutexes */
    status = tx_mutex_create(&switch_mutex_handle,                 "switch_mutex",                 TX_INHERIT);
    TX_CHECK(status);
    status = tx_mutex_create(&phy_mutex_handle,                    "phy_mutex",                    TX_INHERIT);
    TX_CHECK(status);
    status = tx_mutex_create(&ptp_client_event_queue_mutex_handle, "ptp_client_event_queue_mutex", TX_INHERIT);
    TX_CHECK(status);

    /* Create semaphores */

    /* Create event flags */
    status = tx_event_flags_create(&sequencer_events_handle,         "sequencer_events_handle");
    TX_CHECK(status);
    status = tx_event_flags_create(&switch_events_handle,            "switch_events_handle");
    TX_CHECK(status);
    status = tx_event_flags_create(&phy_events_handle,               "phy_events_handle");
    TX_CHECK(status);
    status = tx_event_flags_create(&link_events_handle,              "link_events_handle");
    TX_CHECK(status);
#if FEAT_STP
    status = tx_event_flags_create(&stp_events_handle,               "stp_events_handle");
    TX_CHECK(status);
#endif
#if FEAT_PTP
    status = tx_event_flags_create(&ptp_events_group,                "ptp_events_group");
    TX_CHECK(status);
    status = tx_event_flags_create(&ptp_tx_events_group,             "ptp_tx_events_group");
    TX_CHECK(status);
    status = tx_event_flags_create(&ptp_mac_sync_events_group,       "ptp_mac_sync_events_group");
    TX_CHECK(status);
    status = tx_event_flags_create(&ptp_clock_events_group,          "ptp_clock_events_group");
    TX_CHECK(status);
#endif
    status = tx_event_flags_create(&background_thread_events_handle, "background_thread_events_handle");
    TX_CHECK(status);

    /* Create queues */
#if FEAT_PTP
    status = tx_queue_create(&ptp_event_queue,    "ptp_event_queue",    PTP_CLIENT_MSG_SIZE_WORDS, ptp_event_queue_stack,    sizeof(ptp_event_queue_stack));
    TX_CHECK(status);
    status = tx_queue_create(&ptp_tx_queue,       "ptp_tx_queue",       PTP_PACKET_MSG_SIZE_WORDS, ptp_tx_queue_stack,       sizeof(ptp_tx_queue_stack));
    TX_CHECK(status);
    status = tx_queue_create(&ptp_rx_queue,       "ptp_rx_queue",       PTP_PACKET_MSG_SIZE_WORDS, ptp_rx_queue_stack,       sizeof(ptp_rx_queue_stack));
    TX_CHECK(status);
    status = tx_queue_create(&ptp_clock_queue,    "ptp_clock_queue",    PTP_PACKET_MSG_SIZE_WORDS, ptp_clock_queue_stack,    sizeof(ptp_clock_queue_stack));
    TX_CHECK(status);
    status = tx_queue_create(&ptp_mac_sync_queue, "ptp_mac_sync_queue", PTP_PACKET_MSG_SIZE_WORDS, ptp_mac_sync_queue_stack, sizeof(ptp_mac_sync_queue_stack));
    TX_CHECK(status);
#endif

    /* Create threads */
    status = tx_thread_create(&sequencer_thread_handle,       "state_machine_thread",   (void (*)(ULONG)) sequencer_thread_entry,       thread_number++, sequencer_thread_stack,       SEQUENCER_THREAD_STACK_SIZE,       STATE_MACHINE_THREAD_PRIORITY,   STATE_MACHINE_THREAD_PRIORITY,    TX_NO_TIME_SLICE, TX_AUTO_START);
    TX_CHECK(status);
    status = tx_thread_create(&nx_link_thread_handle,         "nx_link_thread",         (void (*)(ULONG)) nx_link_thread_entry,         thread_number++, nx_link_thread_stack,         NX_LINK_THREAD_STACK_SIZE,         NX_LINK_THREAD_PRIORITY,         NX_LINK_THREAD_PRIORITY,          TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
    status = tx_thread_create(&switch_thread_handle,          "switch_thread",          (void (*)(ULONG)) switch_thread_entry,          thread_number++, switch_thread_stack,          SWITCH_THREAD_STACK_SIZE,          SWITCH_THREAD_PRIORITY,          SWITCH_THREAD_PRIORITY,           TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
    status = tx_thread_create(&phy_thread_handle,             "phy_thread",             (void (*)(ULONG)) phy_thread_entry,             thread_number++, phy_thread_stack,             PHY_THREAD_STACK_SIZE,             PHY_THREAD_PRIORITY,             PHY_THREAD_PRIORITY,              TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
#if FEAT_STP
    status = tx_thread_create(&stp_thread_handle,             "stp_thread",             (void (*)(ULONG)) stp_thread_entry,             thread_number++, stp_thread_stack,             STP_THREAD_STACK_SIZE,             STP_THREAD_PRIORITY,             STP_THREAD_PREMPTION_PRIORITY,    TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
#endif
#if FEAT_COMMS
    status = tx_thread_create(&comms_thread_handle,           "comms_thread",           (void (*)(ULONG)) comms_thread_entry,           thread_number++, comms_thread_stack,           COMMS_THREAD_STACK_SIZE,           COMMS_THREAD_PRIORITY,           COMMS_THREAD_PREMPTION_PRIORITY,  TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
#endif
#if FEAT_PTP
    status = tx_thread_create(&ptp_event_thread,              "ptp_event_thread",       (void (*)(ULONG)) ptp_event_thread_entry,       thread_number++, ptp_event_thread_stack,       PTP_EVENT_THREAD_STACK_SIZE,       PTP_EVENT_THREAD_PRIORITY,       PTP_EVENT_THREAD_PRIORITY,        TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
    status = tx_thread_create(&ptp_tx_thread,                 "ptp_tx_thread",          (void (*)(ULONG)) ptp_tx_thread_entry,          thread_number++, ptp_tx_thread_stack,          PTP_TX_THREAD_STACK_SIZE,          PTP_TX_THREAD_PRIORITY,          PTP_TX_THREAD_PRIORITY,           TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
    status = tx_thread_create(&ptp_rx_thread,                 "ptp_rx_thread",          (void (*)(ULONG)) ptp_rx_thread_entry,          thread_number++, ptp_rx_thread_stack,          PTP_RX_THREAD_STACK_SIZE,          PTP_RX_THREAD_PRIORITY,          PTP_RX_THREAD_PRIORITY,           TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
    status = tx_thread_create(&ptp_clock_thread,              "ptp_clock_thread",       (void (*)(ULONG)) ptp_clock_thread_entry,       thread_number++, ptp_clock_thread_stack,       PTP_CLOCK_THREAD_STACK_SIZE,       PTP_CLOCK_THREAD_PRIORITY,       PTP_CLOCK_THREAD_PRIORITY,        TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
    status = tx_thread_create(&ptp_mac_sync_thread,           "ptp_mac_sync_thread",    (void (*)(ULONG)) ptp_mac_sync_thread_entry,    thread_number++, ptp_mac_sync_thread_stack,    PTP_MAC_SYNC_THREAD_STACK_SIZE,    PTP_MAC_SYNC_THREAD_PRIORITY,    PTP_MAC_SYNC_THREAD_PRIORITY,     TX_NO_TIME_SLICE, TX_DONT_START);
    TX_CHECK(status);
#endif
    status = tx_thread_create(&background_thread_handle,      "background_thread",      (void (*)(ULONG)) background_thread_entry,      thread_number++, background_thread_stack,      BACKGROUND_THREAD_STACK_SIZE,      BACKGROUND_THREAD_PRIORITY,      BACKGROUND_THREAD_PRIORITY,       TX_NO_TIME_SLICE, TX_AUTO_START);
    TX_CHECK(status);

    /* Any threads that call secure functions must allocate secure stack */
    status = tx_thread_secure_stack_allocate(&sequencer_thread_handle,       MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
    status = tx_thread_secure_stack_allocate(&nx_link_thread_handle,         MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
    status = tx_thread_secure_stack_allocate(&switch_thread_handle,          MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
    status = tx_thread_secure_stack_allocate(&phy_thread_handle,             MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
#if FEAT_STP
    status = tx_thread_secure_stack_allocate(&stp_thread_handle,             MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
#endif
#if FEAT_COMMS
    status = tx_thread_secure_stack_allocate(&comms_thread_handle,           MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
#endif
#if FEAT_PTP
    status = tx_thread_secure_stack_allocate(&ptp_event_thread,              MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
    status = tx_thread_secure_stack_allocate(&ptp_tx_thread,                 MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
    status = tx_thread_secure_stack_allocate(&ptp_rx_thread,                 MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
    status = tx_thread_secure_stack_allocate(&ptp_clock_thread,              MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
    status = tx_thread_secure_stack_allocate(&ptp_mac_sync_thread,           MIN(DEFAULT_SECURE_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM));
    TX_CHECK(status);
#endif
    status = tx_thread_secure_stack_allocate(&background_thread_handle,      MIN(BACKGROUND_THREAD_STACK_SIZE, TX_THREAD_SECURE_STACK_MAXIMUM)); /* More stack required for secure background tasks */
    TX_CHECK(status);

    /* Assign loggers to threads */
#ifndef TX_TIMER_PROCESS_IN_ISR
    extern TX_THREAD _tx_timer_thread;
    _tx_timer_thread.logger            = &hlog_system;
#endif
    sequencer_thread_handle.logger = &hlog_generic;
    nx_link_thread_handle.logger       = &hlog_network;
    switch_thread_handle.logger        = &hlog_sw;
    phy_thread_handle.logger           = &hlog_phy;
#if FEAT_STP
    stp_thread_handle.logger           = &hlog_network;
#endif
#if FEAT_COMMS
    comms_thread_handle.logger         = &hlog_comms;
#endif
#if FEAT_PTP
    ptp_event_thread.logger            = &hlog_ptp;
    ptp_tx_thread.logger               = &hlog_ptp;
    ptp_rx_thread.logger               = &hlog_ptp;
    ptp_clock_thread.logger            = &hlog_ptp;
    ptp_mac_sync_thread.logger         = &hlog_ptp;
#endif
    background_thread_handle.logger    = &hlog_generic;

    /* Create timers */
    status = tx_timer_create(&switch_maintenance_timer,    "switch_maintenance_timer",    switch_maintenance_timer_callback,    0, SWITCH_MAINTENANCE_INTERVAL,          SWITCH_MAINTENANCE_INTERVAL,          TX_NO_ACTIVATE);
    TX_CHECK(status);
    status = tx_timer_create(&switch_publish_timer,        "switch_publish_timer",        switch_publish_timer_callback,        0, SWITCH_PUBLISH_STATS_INTERVAL,        SWITCH_PUBLISH_STATS_INTERVAL,        TX_NO_ACTIVATE);
    TX_CHECK(status);
    status = tx_timer_create(&link_check_timer,            "link_check_timer",            link_check_timer_callback,            0, NX_APP_LINK_CHECK_WHEN_DOWN_INTERVAL, NX_APP_LINK_CHECK_WHEN_DOWN_INTERVAL, TX_NO_ACTIVATE);
    TX_CHECK(status);
#if FEAT_DHCP_RESTORE
    status = tx_timer_create(&dhcp_save_timer,             "dhcp_save_timer",             dhcp_save_timer_callback,             0, DHCP_RECORD_SAVE_INTERVAL,            DHCP_RECORD_SAVE_INTERVAL,            TX_NO_ACTIVATE);
    TX_CHECK(status);
#endif
#if FEAT_PTP
#if PTP_PRINT_TIME_INTERVAL
    status = tx_timer_create(&ptp_events_print_time_timer, "ptp_events_print_time_timer", ptp_events_print_time_timer_callback, 0, PTP_PRINT_TIME_INTERVAL,              PTP_PRINT_TIME_INTERVAL,              TX_NO_ACTIVATE);
    TX_CHECK(status);
#endif
    status = tx_timer_create(&ptp_sync_timeout_timer,      "ptp_sync_timeout_timer",      ptp_sync_timeout_timer_callback,      0, PTP_SYNC_TIMEOUT,                     0,                                    TX_NO_ACTIVATE);
    TX_CHECK(status);
    status = tx_timer_create(&ptp_mac_sync_timer,          "ptp_mac_sync_timer",          ptp_mac_sync_timer_callback,          0, PTP_MAC_SYNC_INTERVAL,                PTP_MAC_SYNC_INTERVAL,                TX_NO_ACTIVATE);
    TX_CHECK(status);
#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)
    status = tx_timer_create(&ptp_switch_sync_timer,       "ptp_switch_sync_timer",       ptp_switch_sync_timer_callback,       0, PTP_SWITCH_SYNC_INTERVAL,             PTP_SWITCH_SYNC_INTERVAL,             TX_NO_ACTIVATE);
    TX_CHECK(status);
#endif
#if FEAT_PTP_PPS_SOFT
    status = tx_timer_create(&ptp_pps_pulse_timer,         "ptp_pps_pulse_timer",         ptp_pps_pulse_end_callback,           0, PTP_PPS_SOFT_PULSE_DURATION,          PTP_PPS_SOFT_PULSE_DURATION,          TX_NO_ACTIVATE);
    TX_CHECK(status);
#endif
#endif
    status = tx_timer_create(&background_thread_timer,     "background_thread_timer",     background_thread_timer_callback,     0, BACKGROUND_THREAD_INTERVAL,           BACKGROUND_THREAD_INTERVAL,           TX_NO_ACTIVATE);
    TX_CHECK(status);
}

// clang-format on
