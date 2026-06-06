/*
 * ptp_event_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "ptp.h"
#include "utils.h"
#include "validation.h"


SHORT ptp_utc_offset = 0;

TX_THREAD ptp_event_thread_handle;
uint8_t   ptp_event_thread_stack[PTP_EVENT_THREAD_STACK_SIZE];

TX_QUEUE ptp_event_queue_handle;
uint32_t ptp_event_queue_stack[PTP_EVENT_QUEUE_SIZE * PTP_CLIENT_MSG_SIZE_WORDS];

TX_EVENT_FLAGS_GROUP ptp_events_handle;

#if PTP_PRINT_TIME_INTERVAL
TX_TIMER ptp_events_print_time_timer;
#endif
TX_TIMER ptp_sync_timeout_timer;

ptp_event_counters_t ptp_event_counters;

volatile atomic_uint_fast8_t ptp_port_connected_to_master = PORT_HOST;
static volatile atomic_bool  new_master_waiting_for_sync  = false;
static uint8_t               grandmaster_identity[NX_PTP_CLOCK_PORT_IDENTITY_SIZE];


/* Store master into event struct, including pointed to information so it can be reconstructed after being queued */
static inline void ptp_pack_master_message(ptp_client_event_info_t *event_info, NX_PTP_CLIENT_MASTER *master) {

    event_info->master = *master;

    memcpy(event_info->_grandmaster_identity,
           master->nx_ptp_client_master_grandmaster_identity,
           NX_PTP_CLOCK_PORT_IDENTITY_SIZE);
}


/* Reconstruct master from an event struct, so important pointers aren't left dangling */
static inline void ptp_unpack_master_message(ptp_client_event_info_t *event_info) {
    event_info->master.nx_ptp_client_master_grandmaster_identity = event_info->_grandmaster_identity;
}


/* Event callback for NetX PTP client */
UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data) {

    tx_status_t             status = TX_SUCCESS;
    ptp_client_event_info_t event_info;
    port_index_t            port = (port_index_t) callback_data;

    assert(port < NUM_PHYS);
    assert(ptp_client_ptr == &ptp_client[port]);

    event_info.event = event;
    event_info.port  = port;

    /* The event_data must be copied since the caller may overwrite the memory
     * when this function has returned. */
    switch (event_info.event) {

        case PTP_CLIENT_EVENT_MASTER: {
            ptp_pack_master_message(&event_info, (NX_PTP_CLIENT_MASTER *) event_data);
            break;
        }

        case PTP_CLIENT_EVENT_SYNC: {
            event_info.sync = *((NX_PTP_CLIENT_SYNC *) event_data);
            break;
        }

        /* No event data */
        case PTP_CLIENT_EVENT_TIMEOUT: {
            break;
        }

        /* Unknown event */
        default: {
            error_handler();
        }
    }

    /* Send the event to the queue */
    status = ptp_client_event_queue_send(&event_info);
    TX_CHECK(status);

    return NX_SUCCESS;
}


#if PTP_PRINT_TIME_INTERVAL

void ptp_events_print_time_timer_callback(uint32_t id) {
    tx_status_t status = tx_event_flags_set(&ptp_events_handle, PTP_EVENT_PRINT_TIME, TX_OR);
    TX_CHECK(status);
}

#endif


void ptp_sync_timeout_timer_callback(uint32_t id) {

    tx_status_t status = TX_SUCCESS;

    if (new_master_waiting_for_sync && (ptp_port_connected_to_master != PORT_HOST)) {
        new_master_waiting_for_sync = false;
        status                      = tx_event_flags_set(&ptp_events_handle, PTP_EVENT_MASTER_SYNC_TIMEOUT, TX_OR);
        TX_CHECK(status);
    }
}


static tx_status_t ptp_sync_timeout_reset() {

    tx_status_t status = TX_SUCCESS;

    new_master_waiting_for_sync = false;

    status = tx_timer_deactivate(&ptp_sync_timeout_timer);
    if (status != TX_SUCCESS) return status;

    return status;
}


static tx_status_t ptp_sync_timeout_start() {

    tx_status_t status = TX_SUCCESS;

    // TODO: when clock adjusting is done re-enable this function
    // Since there is no adjusting, the offset from the master is
    // greater than the maximum allowed gPTP delay (800ns) and so
    // a SYNC is never triggered

    // new_master_waiting_for_sync = true;

    // status = tx_timer_change(&ptp_sync_timeout_timer, PTP_SYNC_TIMEOUT, 0);
    // if (status != TX_SUCCESS) return status;
    // status = tx_timer_activate(&ptp_sync_timeout_timer);
    // if (status != TX_SUCCESS) return status;

    return status;
}


/* This Thread starts the PTP client, and prints status information */
void ptp_event_thread_entry(uint32_t initial_input) {

    TX_INTERRUPT_SAVE_AREA

    nx_status_t nx_status = NX_SUCCESS;
    tx_status_t tx_status = TX_SUCCESS;

    NX_PTP_TIME      time;
    NX_PTP_DATE_TIME date;

    uint32_t                event_flags;
    ptp_client_event_info_t event_info;
    bool                    queue_empty;

    uint8_t port_identity[NX_PTP_CLOCK_PORT_IDENTITY_SIZE]; /* 64-bit EUI + port */

    bool    new_master;
    int32_t clock_comparison;

    NX_PTP_CLIENT_MASTER master; /* Note: the nx_ptp_client_master_address and nx_ptp_client_master_port_identity fields are ALWAYS INVALID (due to copying struct with pointers) */

    static_assert(PTP_VLAN == 0, "PTP Currently only supported on VLAN 0");

    /* Reset variables */
    ptp_port_connected_to_master = PORT_HOST;
    new_master_waiting_for_sync  = false;

    /* Enable master mode initially on all ports with the lowest priority so
     * it is only used as a last restort. */
    nx_status = ptp_client_restart_all();
    NX_CHECK(nx_status);

#if PTP_PRINT_TIME_INTERVAL

    /* Start the time print timer */
    tx_status = tx_timer_activate(&ptp_events_print_time_timer);
    TX_CHECK(tx_status);

#endif

    while (1) {

        tx_status = tx_event_flags_get(&ptp_events_handle, PTP_EVENT_ALL, TX_OR_CLEAR, &event_flags, TX_WAIT_FOREVER);
        TX_CHECK(tx_status);

        /* Receive events from the event queue */
        if (event_flags & PTP_EVENT_CLIENT) {
            do {

                /* Receive event */
                tx_status = tx_queue_receive(&ptp_event_queue_handle, &event_info, TX_NO_WAIT);
                if (tx_status == TX_SUCCESS) {
                    switch (event_info.event) {

                        case PTP_CLIENT_EVENT_MASTER: {

                            ptp_unpack_master_message(&event_info);
                            ptp_sync_timeout_reset();
                            new_master = false;

                            /* No master assigned, take the first one to appear */
                            if (ptp_port_connected_to_master == PORT_HOST) {
                                new_master = true;

                                /* Ensure there is a corresponding sync event */
                                tx_status = ptp_sync_timeout_start();
                                TX_CHECK(tx_status);
                            }

                            /* There is a master assigned, need to compare clocks */
                            else {

                                clock_comparison = nx_ptp_client_master_clock_compare(&event_info.master, &master);

                                /* If the new master is better than the current one then use it */
                                if (clock_comparison > 0) {
                                    new_master                  = true;
                                    new_master_waiting_for_sync = true;

                                    /* Ensure there is a corresponding sync event */
                                    tx_status = ptp_sync_timeout_start();
                                    TX_CHECK(tx_status);
                                }

                                /* When the client times out it will update itself to be the master with its worse clock */
                                else if ((clock_comparison < 0) && (event_info.port == ptp_port_connected_to_master)) {
                                    new_master = true;
                                    ptp_event_counters.master_timeout++;
                                }

                                /* New master on port connected to master with same grand master. This shouldn't be
                                 * possible so do a full PTP restart */
                                else if ((clock_comparison == 0) && (event_info.port == ptp_port_connected_to_master)) {
                                    LOG_WARNING("PTP: New identical master on port connected to master (%d)", event_info.port);
                                    nx_status = ptp_client_restart_all();
                                    NX_CHECK(nx_status);
                                }
                            }

                            /* Nothing to do */
                            if (!new_master) break;

                            TX_DISABLE

                            /* Save the new master */
                            ptp_port_connected_to_master = event_info.port;
                            master                       = event_info.master;
                            memcpy(grandmaster_identity, event_info.master.nx_ptp_client_master_grandmaster_identity, NX_PTP_CLOCK_PORT_IDENTITY_SIZE);
                            master.nx_ptp_client_master_grandmaster_identity                                 = grandmaster_identity;
                            ptp_client[event_info.port].ptp_master.nx_ptp_client_master_grandmaster_identity = ptp_client[event_info.port].nx_ptp_client_port_identity;
                            ptp_event_counters.new_master++;

                            TX_RESTORE

                            /* Flush events from the non-master port */
                            tx_status = ptp_client_filter_event_queue(1UL << event_info.port, PORT_BITS_ALL);
                            TX_CHECK(tx_status);

                            /* Set port EUI for upcoming client initialisations */
                            write_port_identity_eui(port_identity);

                            /* Propagate master info to other clients and restart them */
                            for (port_index_t i = PORT0; i < NUM_PHYS; i++) {

                                /* Skip the master port */
                                if (i == event_info.port) continue;

                                /* Reset the client */
                                nx_status = ptp_client_reset(i, false);
                                NX_CHECK(nx_status);

                                /* Apply the new master config */
                                nx_status = nx_ptp_client_master_enable(
                                    &ptp_client[i],
                                    NX_PTP_CLIENT_ROLE_SLAVE_AND_MASTER,
                                    master.nx_ptp_client_master_priority1,
                                    master.nx_ptp_client_master_priority2,
                                    master.nx_ptp_client_master_clock_class,
                                    master.nx_ptp_client_master_clock_accuracy,
                                    master.nx_ptp_client_master_offset_scaled_log_variance,
                                    master.nx_ptp_client_master_steps_removed + 1,
                                    NX_PTP_MASTER_TIME_SRC_PTP);
                                NX_CHECK(nx_status);

                                /* Inject the grandmaster's identity */
                                ptp_client[i].ptp_master.nx_ptp_client_master_grandmaster_identity = grandmaster_identity;

                                write_port_identity_number(port_identity, i);
                                nx_status = ptp_client_start(i, port_identity);
                                NX_CHECK(nx_status);
                            }

                            LOG_INFO("PTP: New master clock on port %d, grandmaster = %02x:%02x:%02x:%02x:%02x:%02x",
                                     event_info.port,
                                     grandmaster_identity[0],
                                     grandmaster_identity[1],
                                     grandmaster_identity[2],
                                     grandmaster_identity[5],
                                     grandmaster_identity[6],
                                     grandmaster_identity[7]);

                            break;
                        }

                        case PTP_CLIENT_EVENT_SYNC: {

                            VAL_FAULT_BREAK(PTP, RX_FILTER_DROP_META, VAL_1_IN_10);

                            new_master_waiting_for_sync = false;

                            ptp_event_counters.sync++;
                            nx_ptp_client_sync_info_get(&event_info.sync, NX_NULL, &ptp_utc_offset);
                            LOG_INFO("PTP: SYNC event, utc offset=%d", ptp_utc_offset);
                            break;
                        }

                        case PTP_CLIENT_EVENT_TIMEOUT: {

                            if (event_info.port != ptp_port_connected_to_master) {
                                LOG_WARNING("PTP: Timeout from non-master port %d", event_info.port);
                                break;
                            }

                            ptp_port_connected_to_master = PORT_HOST;
                            ptp_event_counters.master_timeout++;

                            /* Restart all clients to begin looking for a new master */
                            nx_status = ptp_client_restart_all();
                            NX_CHECK(nx_status);

                            LOG_INFO("PTP: Master clock timeout on port %d", event_info.port);
                            break;
                        }

                        default: {
                            LOG_WARNING("PTP: Unknown event");
                            error_handler();
                        }
                    }
                    queue_empty = false;
                }

                else if (tx_status != TX_STATUS_QUEUE_EMPTY) {
                    error_handler();
                }

                else {
                    queue_empty = true;
                }
            } while (!queue_empty);
        }

        /* Sync missed */
        if (event_flags & PTP_EVENT_MASTER_SYNC_TIMEOUT) {
            VAL_COVER(PTP, MASTER_SYNC_TIMEOUT);
            ptp_notify_port_down(ptp_port_connected_to_master);
            LOG_WARNING("PTP: No SYNC event after new external master");
        }

#if PTP_PRINT_TIME_INTERVAL

        /* Get, convert, and print the PTP time */
        if (event_flags & PTP_EVENT_PRINT_TIME) {

            /* Get time from the STM32's MAC */
            ptp_mac_get_time(&time);

            /* Convert and print time */
            nx_status = nx_ptp_client_utility_convert_time_to_date(
                &time,
                (time.second_high == 0)
                    ? -CONSTRAIN(ptp_utc_offset, 0, time.second_low)
                    : -ptp_utc_offset, /* Prevent the offset from making the time negative */
                &date);
            NX_CHECK(nx_status);
            LOG_INFO("PTP: Time is %2u/%02u/%u %02u:%02u:%02u.%09lu (UTC)",
                     date.day, date.month, date.year,
                     date.hour, date.minute, date.second, date.nanosecond);
        }

#endif
    }
}
