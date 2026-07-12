/*
 * ptp_clock_thread.c
 *
 *  Created on: May 26, 2026
 *      Author: bens1
 */

#include "stdint.h"
#include "stdbool.h"
#include "assert.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "ptp.h"
#include "switch.h"
#include "utils.h"
#include "validation.h"


TX_THREAD ptp_clock_thread;
uint8_t   ptp_clock_thread_stack[PTP_CLOCK_THREAD_STACK_SIZE];

TX_EVENT_FLAGS_GROUP ptp_clock_events_group;

TX_QUEUE ptp_clock_queue;
uint32_t ptp_clock_queue_stack[PTP_CLOCK_QUEUE_SIZE * PTP_PACKET_MSG_SIZE_WORDS];

#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)

TX_TIMER ptp_switch_sync_timer;


static void ptp_init_switch_sync_controllers(ptp_controller_t *controllers, uint32_t time, int32_t *corrections) {

    corrections[SWITCH0] = 0;

    for (switch_index_t i = SWITCH1; i < NUM_SWITCHES; i++) {
        ptp_pi_controller_init(&controllers[i - 1],
                               PTP_SWITCH_SYNC_CONTROLLER_KP,
                               PTP_SWITCH_SYNC_CONTROLLER_KI,
                               PTP_SWITCH_SYNC_CONTROLLER_INTEGRAL_MAX,
                               time);
        corrections[i] = 0;
    }
}

#endif


UINT ptp_clock_callback(NX_PTP_CLIENT *client_ptr, UINT operation,
                        NX_PTP_TIME *time_ptr, NX_PACKET *packet_ptr,
                        VOID *callback_data) {

    tx_status_t      tx_status     = TX_SUCCESS;
    nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;
    port_index_t     port          = (port_index_t) callback_data;

    assert(port < NUM_PHYS);
    assert(client_ptr == &ptp_client[port]);

    switch (operation) {

        /* No initialisation to do */
        case NX_PTP_CLIENT_CLOCK_INIT:
            break;

        /* Set all switch clocks and stm32 clock if this client is connected to
         * the grandmaster
         *
         * Note: This should be done as little as possible since it can corrupt
         *       time stamps.
         */
        case NX_PTP_CLIENT_CLOCK_SET:

            if (port == ptp_port_connected_to_master) {

                VAL_COVER(PTP_CLOCK, CB_SET);

                TX_THREAD *current_thread = tx_thread_identify();
                UINT       old_priority;

                assert(current_thread != TX_NULL);

                /* Raise the priority of this thread to prevent interruptions */
                tx_status = tx_thread_priority_change(current_thread, TX_TIMER_THREAD_PRIORITY + 1, &old_priority);
                TX_CHECK(tx_status);

                /* A set implies a new master or catastrophic desync so reset the clock rates */
                switch_status = switch_set_rate_all(SJA1105_PTP_CLK_RATE_DEFAULT, NULL);
                SWITCH_CHECK(switch_status);

                /* Set the switch time(s). Apply a correction for how long it takes to set */
                static NX_PTP_TIME correction = {
                    .second_high = 0,
                    .second_low  = 0,
                    .nanosecond  = (((NUM_SWITCHES - 1) * SJA1105_WRITE_TIME(1)) + /* Set rate (only needed for non-host switch due to switch_set_rate_all caching) */
                                   SJA1105_READ_TIME(2) +                         /* Read current time */
                                   SJA1105_WRITE_TIME(1) +                        /* Write PTP Control reg */
                                   SJA1105_WRITE_TIME(2) +                        /* Write new time */
                                   US_TO_NS(1)                                    /* Plus some time to execute instructions */
                                   )};
                nx_status = _nx_ptp_client_utility_time_sum(time_ptr, &correction, time_ptr);
                NX_CHECK(nx_status);
                switch_status = switch_set_time_all(time_ptr);
                SWITCH_CHECK(switch_status);

                /* Reset the priority */
                tx_status = tx_thread_priority_change(current_thread, old_priority, &old_priority);
                TX_CHECK(tx_status);

                /* Reset the PI controller in the clock thread */
                tx_status = tx_event_flags_set(&ptp_clock_events_group, PTP_CLOCK_EVENT_RESET, TX_OR);
                TX_CHECK(tx_status);

                /* Copy the time from the switch back to the STM32's MAC */
                tx_status = tx_event_flags_set(&ptp_mac_sync_events_group, PTP_CLOCK_EVENT_MAC_SYNC, TX_OR);
                TX_CHECK(tx_status);
            }

            break;

        /* Extract timestamp from packet */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_EXTRACT:

            VAL_COVER(PTP_CLOCK, CB_TS_EXTRACT);

            /* Return timestamp stored at the beginning of the packet */
            ptp_packet_extract_timestamp(packet_ptr, time_ptr);
            break;

        /* Get clock */
        case NX_PTP_CLIENT_CLOCK_GET:

            VAL_COVER(PTP_CLOCK, CB_GET);

            /* Use the local PTP clock which the application synchronises to SWITCH0 */
            ptp_mac_get_time(time_ptr);
            break;

        /* Adjust clock */
        case NX_PTP_CLIENT_CLOCK_ADJUST:

            if (port == ptp_port_connected_to_master) {

                VAL_COVER(PTP_CLOCK, CB_ADJUST);

                /* Send the event for the clock thread to process */
                ptp_packet_event_info_t event_info = {
                    .event = PTP_CLOCK_EVENT_ADJUST,
                    .port  = port,
                    .time  = *time_ptr,
                };
                tx_status = tx_queue_send(&ptp_clock_queue, &event_info, TX_NO_WAIT);
                if (tx_status == TX_QUEUE_FULL) {

                    VAL_COVER(PTP_CLOCK, QUEUE_FULL);

                    /* If adjustments have built up then flush them because
                     * only the most recent one is valid.
                     * Note: Only the PTP client connected to the master can
                     *       put stuff in the clock queue, so it won't be full
                     *       the second time */
                    tx_status = tx_queue_flush(&ptp_clock_queue);
                    TX_CHECK(tx_status);
                    tx_status = tx_queue_send(&ptp_clock_queue, &event_info, TX_NO_WAIT);
                    TX_CHECK(tx_status);
                }

                /* Error */
                else if (tx_status != TX_SUCCESS) {
                    error_handler();
                }

                /* Notify the clock thread */
                tx_status = tx_event_flags_set(&ptp_clock_events_group, PTP_CLOCK_EVENT_ADJUST, TX_OR);
                TX_CHECK(tx_status);
            }

            break;

        /* Prepare timestamp for current packet. Not used because the timestamp
         * needs to be immediately fetched from the SJA1105. This is handled by
         * the application */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_PREPARE:
            break;

        /* Update soft timer. Not used by hardware callback function */
        case NX_PTP_CLIENT_CLOCK_SOFT_TIMER_UPDATE:
            break;

        default:
            error_handler();
            break;
    }

    return nx_status;
}


#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)

void ptp_switch_sync_timer_callback(uint32_t id) {
    tx_status_t status = tx_event_flags_set(&ptp_clock_events_group, PTP_CLOCK_EVENT_SWITCH_SYNC, TX_OR);
    TX_CHECK(status);
}

#endif


void ptp_clock_thread_entry(uint32_t initial_input) {

    static_assert(PTP_CLOCK_CONTROLLER_KP >= 0);
    static_assert(PTP_CLOCK_CONTROLLER_KI >= 0);
    static_assert(PTP_CLOCK_CONTROLLER_INTEGRAL_MAX >= 0);
    static_assert(PTP_CLOCK_CONTROLLER_OUTPUT_MAX <= INT32_MAX); /* For typeof(controller_output) = int32_t */

#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)
    static_assert(PTP_SWITCH_SYNC_CONTROLLER_KP >= 0);
    static_assert(PTP_SWITCH_SYNC_CONTROLLER_KI >= 0);
    static_assert(PTP_SWITCH_SYNC_CONTROLLER_INTEGRAL_MAX >= 0);
    static_assert(PTP_SWITCH_SYNC_CONTROLLER_OUTPUT_MAX <= INT32_MAX); /* For typeof(controller_output) = int32_t */
#endif

    tx_status_t      tx_status     = TX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    uint32_t                event_flags;
    ptp_packet_event_info_t event_info;

    ptp_controller_t clock_controller          = {.initialised = false};
    uint8_t          clock_controller_cooldown = 0;
    int32_t          controller_output;
    uint32_t         rate;

    uint32_t time_current;

#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)
    uint32_t         time_last_rate_write = 0;
    NX_PTP_TIME      offset;
    ptp_controller_t switch_sync_controllers[NUM_SWITCHES - 1];
    int32_t          rate_corrections[NUM_SWITCHES] = {[SWITCH0] = 0};
#else
    static const int32_t *rate_corrections = NULL;
#endif

#if DEBUG
    uint32_t enqueued;
    uint32_t queue_high_water_mark = 0;
#endif

#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)

    /* Set the switch sync controllers to uninitialised */
    for (switch_index_t i = SWITCH1; i < NUM_SWITCHES; i++) {
        switch_sync_controllers[i - 1].initialised = false;
    }

    /* Start the switch sync timer */
    tx_status = tx_timer_activate(&ptp_switch_sync_timer);
    TX_CHECK(tx_status);

#endif

    while (1) {

        /* Wait for an event */
        tx_status = tx_event_flags_get(&ptp_clock_events_group, PTP_EVENT_ALL, TX_OR_CLEAR, &event_flags, TX_WAIT_FOREVER);
        TX_CHECK(tx_status);

        /* Reset the control loop */
        if (event_flags & PTP_CLOCK_EVENT_RESET) {

            time_current = tx_time_get_ms();

            /* Reset clock controller */
            ptp_pi_controller_init(&clock_controller,
                                   PTP_CLOCK_CONTROLLER_KP,
                                   PTP_CLOCK_CONTROLLER_KI,
                                   PTP_CLOCK_CONTROLLER_INTEGRAL_MAX,
                                   time_current);

#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)

            /* Reset switch sync controller(s) */
            ptp_init_switch_sync_controllers(switch_sync_controllers, time_current, rate_corrections);

#endif
        }

        /* Adjust the time */
        if (event_flags & PTP_CLOCK_EVENT_ADJUST) {
            while (1) {

                /* Get an adjust event from the queue */
                tx_status = tx_queue_receive(&ptp_clock_queue, &event_info, TX_NO_WAIT);
                if (tx_status == TX_QUEUE_EMPTY) {
                    break;
                } else if (tx_status != TX_SUCCESS) {
                    error_handler();
                }

#if DEBUG

                tx_status = tx_queue_info_get(&ptp_clock_queue, NULL, &enqueued, NULL, NULL, NULL, NULL);
                TX_CHECK(tx_status);
                if ((enqueued + 1) > queue_high_water_mark) {
                    queue_high_water_mark = (enqueued + 1);
                    LOG_INFO("PTP: Clock Queue high water mark = %lu/%d", queue_high_water_mark, PTP_CLOCK_QUEUE_SIZE);
                }

#endif

                assert(event_info.event == PTP_CLOCK_EVENT_ADJUST);
                assert(event_info.time.second_high == 0);
                assert(event_info.time.second_low == 0);
                assert(ABS(event_info.time.nanosecond) <= S_TO_NS(1)); /* Error should be less than 1s */

                /* Inject 500ms of lag to test queue overflow handling */
                VAL_FAULT_CHANCE(PTP_CLOCK, THREAD_LAG, VAL_1_IN_100, tx_thread_sleep_ms(500));

                /* Discard adjustments after a step */
                if (clock_controller_cooldown > 0) {
                    clock_controller_cooldown--;
                    continue;
                }

                /* For large offsets simply add/subtract */
                if (ABS(event_info.time.nanosecond) > MS_TO_NS(PTP_CLOCK_FINE_ADJUST_THRESHOLD)) {

                    /* Notify the MAC sync thread to reset and ignore the next
                     * few sync timestamps since they could be corrupted by
                     * the switch time step. Do this before stepping the time
                     * so in flight syncs can be invalidated. */
                    tx_status = tx_event_flags_set(&ptp_mac_sync_events_group, PTP_CLOCK_EVENT_RESET, TX_OR);
                    TX_CHECK(tx_status);

                    /* Switch clock(s) add/sub */
                    switch_status = switch_add_ns_all(event_info.time.nanosecond);
                    SWITCH_CHECK(switch_status);

                    /* MAC clock add/sub */
                    NX_PTP_TIME mac_offset = event_info.time;
                    mac_offset.nanosecond  = -mac_offset.nanosecond;
                    ptp_mac_adjust_time_coarse(&mac_offset);

                    /* Clear the integral */
                    ptp_pi_controller_clear(&clock_controller, tx_time_get_ms());

                    /* Clear the rates */
                    switch_status = switch_set_rate_all(SJA1105_PTP_CLK_RATE_DEFAULT, rate_corrections);
                    SWITCH_CHECK(switch_status);

                    clock_controller_cooldown = 1;

                    VAL_COVER(PTP_CLOCK, SWITCH_ADJUST_COARSE);
                }

                /* For small offsets use a PI controller */
                else {

                    /* Initialise if not already done */
                    if (!clock_controller.initialised) {
                        ptp_pi_controller_init(&clock_controller,
                                               PTP_CLOCK_CONTROLLER_KP,
                                               PTP_CLOCK_CONTROLLER_KI,
                                               PTP_CLOCK_CONTROLLER_INTEGRAL_MAX,
                                               tx_time_get_ms());
                    }

                    /* Compute the controller output */
                    controller_output = ptp_pi_controller_compute(&clock_controller, event_info.time.nanosecond, tx_time_get_ms());
                    controller_output = CONSTRAIN(controller_output,
                                                  SJA1105_PTP_RATE_TO_I32(PTP_CLOCK_CONTROLLER_MIN_RATE),
                                                  SJA1105_PTP_RATE_TO_I32(PTP_CLOCK_CONTROLLER_MAX_RATE));

                    /* Convert to fixed point rate */
                    rate = SJA1105_PTP_I32_TO_RATE(controller_output);
                    assert((rate >= PTP_CLOCK_CONTROLLER_MIN_RATE) && (rate <= PTP_CLOCK_CONTROLLER_MAX_RATE));

                    /* Apply the rate */
                    switch_status = switch_set_rate_all(rate, rate_corrections);
                    SWITCH_CHECK(switch_status);
#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)
                    time_last_rate_write = tx_time_get_ms();
#endif
                    VAL_COVER(PTP_CLOCK, SWITCH_ADJUST_FINE);
                }

#if PTP_CLOCK_PRINT_OFFSET
                LOG_INFO("PTP: Clock error = %li ns", event_info.time.nanosecond);
#endif
            }
        }

#if FEAT_PTP_SWITCH_SYNC && (NUM_SWITCHES > 1)

        /* Synchronise the SJA1105 clocks with each other */
        if (event_flags & PTP_CLOCK_EVENT_SWITCH_SYNC) {

            time_current = tx_time_get_ms();

            /* Initialise controllers if not already done */
            if (!switch_sync_controllers[0].initialised) {
                ptp_init_switch_sync_controllers(switch_sync_controllers, time_current, rate_corrections);
            }

            /* Calculate corrections */
            for (switch_index_t i = SWITCH1; i < NUM_SWITCHES; i++) {

                /* Get the offset between the two switches */
                switch_status = switch_get_timestamp_offsets(SWITCH0, i, &offset);
                SWITCH_CHECK(switch_status);

                /* Very out of sync, re-sync (also notify the MAC sync thread that timestamps may be invalid) */
                if ((offset.second_high != 0) || (offset.second_low != 0)) {
                    switch_status = SJA1105_SyncTimestamps(&switch_handles[SWITCH0], &switch_handles[SWITCH1]);
                    SWITCH_CHECK(switch_status);
                    tx_status = tx_event_flags_set(&ptp_clock_events_group, PTP_CLOCK_EVENT_RESET, TX_OR);
                    TX_CHECK(tx_status);
                    continue;
                }

                /* Compute the controller output */
                controller_output = ptp_pi_controller_compute(&switch_sync_controllers[i - 1], offset.nanosecond, time_current);

                /* Convert to fixed point rate */
                rate                = SJA1105_PTP_I32_TO_RATE(controller_output);
                rate                = CONSTRAIN(rate,
                                                PTP_SWITCH_SYNC_CONTROLLER_MIN_RATE,
                                                PTP_SWITCH_SYNC_CONTROLLER_MAX_RATE);
                rate_corrections[i] = SJA1105_PTP_RATE_TO_I32(rate);

#if PTP_SWITCH_SYNC_PRINT_OFFSET
                LOG_INFO("PTP: Switch %d current offset = %li ns", i, offset.nanosecond);
#endif
            }

            /* Apply corrections if the rates haven't been set in a while */
            if ((time_current - time_last_rate_write) > (PTP_SWITCH_SYNC_INTERVAL - 1)) {
                switch_status = switch_set_rate_all(SJA1105_PTP_CLK_RATE_DEFAULT, rate_corrections);
                SWITCH_CHECK(switch_status);
                time_last_rate_write = time_current;
            }
        }

#endif
    }
}
