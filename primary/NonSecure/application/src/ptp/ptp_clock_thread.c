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

#if FEAT_SWITCH_SYNC && (NUM_SWITCHES > 1)
TX_TIMER ptp_switch_sync_timer;
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
                switch_status = switch_set_rate_all(SJA1105_PTP_CLK_RATE_DEFAULT);
                SWITCH_CHECK(switch_status);

                /* Set the switch time(s). Apply a correction for how long it takes to set */
                static NX_PTP_TIME correction = {
                    .second_high = 0,
                    .second_low  = 0,
                    .nanosecond  = ((NUM_SWITCHES * SJA1105_WRITE_TIME(1)) + /* Set rate */
                                   SJA1105_READ_TIME(2) +                   /* Read current time */
                                   SJA1105_WRITE_TIME(1) +                  /* Write PTP Control reg */
                                   SJA1105_WRITE_TIME(2) +                  /* Write new time */
                                   US_TO_NS(1)                              /* Plus some time to execute instructions */
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

                // /* Print the new time for good measure */
                // tx_status = tx_event_flags_set(&ptp_events_group, PTP_EVENT_PRINT_TIME, TX_OR);
                // TX_CHECK(tx_status);
            }

            break;

        /* Extract timestamp from packet */
        case NX_PTP_CLIENT_CLOCK_PACKET_TS_EXTRACT:

            VAL_COVER(PTP_CLOCK, CB_TS_EXTRACT);

            /* Return timestamp stored at the beginning of the packet.  */
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

                    /* Adjustments can be dropped */
                    VAL_COVER(PTP_CLOCK, QUEUE_FULL);
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


#if FEAT_SWITCH_SYNC && (NUM_SWITCHES > 1)

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

    tx_status_t tx_status = TX_SUCCESS;
    // nx_status_t      nx_status     = NX_SUCCESS;
    sja1105_status_t switch_status = SJA1105_OK;

    uint32_t                event_flags;
    ptp_packet_event_info_t event_info;

    ptp_controller_t pi_controller = {.initialised = false};
    int32_t          controller_output;
    uint32_t         rate;

#if DEBUG
    uint32_t enqueued;
    uint32_t queue_high_water_mark = 0;
#endif

#if FEAT_SWITCH_SYNC && (NUM_SWITCHES > 1)

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
            ptp_pi_controller_init(&pi_controller,
                                   PTP_CLOCK_CONTROLLER_KP,
                                   PTP_CLOCK_CONTROLLER_KI,
                                   PTP_CLOCK_CONTROLLER_INTEGRAL_MAX);
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

                /* Inject 500ms of lag to test queue overflow handling TODO: keep? */
                // VAL_FAULT_CHANCE(PTP_CLOCK, THREAD_LAG, VAL_1_IN_100, tx_thread_sleep_ms(500));

                /* For large offsets simply add/subtract */
                if (ABS(event_info.time.nanosecond) > MS_TO_NS(100)) {

                    /* Add/sub */
                    switch_status = switch_add_ns_all(event_info.time.nanosecond);
                    SWITCH_CHECK(switch_status);

                    /* Clear the integral */
                    ptp_pi_controller_clear(&pi_controller);

                    VAL_COVER(PTP_CLOCK, SWITCH_ADJUST_COARSE);
                }

                /* For small offsets use a PI controller */
                else {

                    assert(pi_controller.initialised);

                    /* Compute the controller output */
                    controller_output = ptp_pi_controller_compute(&pi_controller, event_info.time.nanosecond);

                    /* Convert to fixed point rate */
                    rate = controller_output + 0x80000000U;

                    /* Apply the rate */
                    switch_status = switch_set_rate_all(rate);
                    SWITCH_CHECK(switch_status);

                    VAL_COVER(PTP_CLOCK, SWITCH_ADJUST_FINE);
                    // LOG_INFO("PTP: Clock PI output = %li", controller_output);
                }

                LOG_INFO("PTP: Clock error = %li ns", event_info.time.nanosecond);
            }
        }

#if FEAT_SWITCH_SYNC && (NUM_SWITCHES > 1)

        // TODO: move over
        if (event_flags & PTP_CLOCK_EVENT_SWITCH_SYNC) {
        }

#endif
    }
}
