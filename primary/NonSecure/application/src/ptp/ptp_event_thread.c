/*
 * ptp_event_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"

#include "hal.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"
#include "nx_stm32_eth_driver.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "utils.h"
#include "ptp_thread.h"
#include "ptp_init.h"


SHORT ptp_utc_offset = 0;

NX_PTP_CLIENT  ptp_client[NUM_PHYS];
static uint8_t nx_internal_ptp_stack[NUM_PHYS][NX_INTERNAL_PTP_EVENT_THREAD_STACK_SIZE];

TX_THREAD ptp_event_thread_handle;
uint8_t   ptp_event_thread_stack[PTP_EVENT_THREAD_STACK_SIZE];

TX_QUEUE ptp_event_queue_handle;
uint32_t ptp_event_queue_stack[PTP_EVENT_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

ptp_event_counters_t ptp_event_counters;

NX_PTP_CLIENT *connected_to_master = NULL;
uint8_t        grandmaster_identity[NX_PTP_CLOCK_PORT_IDENTITY_SIZE];

// TODO: stop instances when links go down + restart when up
//       also need to elect new master on port down rather than
//       waiting for timeout


/* Convert 48-bit MAC address to 64-bit EUI */
static inline void write_port_identity_eui(uint8_t *port_identity) {
    port_identity[0] = MAC_ADDR_OCTET1;
    port_identity[1] = MAC_ADDR_OCTET2;
    port_identity[2] = MAC_ADDR_OCTET3;
    port_identity[3] = 0xff;
    port_identity[4] = 0xfe;
    port_identity[5] = MAC_ADDR_OCTET4;
    port_identity[6] = MAC_ADDR_OCTET5;
    port_identity[7] = MAC_ADDR_OCTET6;
}


static inline void write_port_identity_number(uint8_t *port_identity, uint16_t number) {
    port_identity[8] = (uint8_t) ((number + 1) >> 8);
    port_identity[9] = (uint8_t) (number + 1); /* Indexed from 1 */
}


/* Note: In this function clients may generate events before other clients have
 *       been started. Since events are serialised and accessed via a queue,
 *       this is not an issue */
static nx_status_t ptp_reset_all_clients() {

    nx_status_t status = NX_SUCCESS;
    uint8_t     port_identity[NX_PTP_CLOCK_PORT_IDENTITY_SIZE]; /* 64-bit EUI + port */

    /* Stop the clients (may not be started) */
    for (port_index_t i = 0; i < NUM_PHYS; i++) {
        status = nx_ptp_client_stop(&ptp_client[i]);
        if ((status != NX_SUCCESS) &&
            (status != NX_PTP_CLIENT_NOT_STARTED)) return status;
    }

    /* Enable the default master configuration */
    for (port_index_t i = 0; i < NUM_PHYS; i++) {
        status = nx_ptp_client_master_enable(
            &ptp_client[i],
            NX_PTP_CLIENT_ROLE_SLAVE_AND_MASTER,
            NX_PTP_CLIENT_MASTER_PRIORITY,
            PTP_CLIENT_MASTER_SUB_PRIORITY,
            NX_PTP_CLIENT_MASTER_CLOCK_CLASS,
            NX_PTP_CLIENT_MASTER_ACCURACY,
            NX_PTP_CLIENT_MASTER_CLOCK_VARIANCE,
            NX_PTP_CLIENT_MASTER_CLOCK_STEPS_REMOVED,
            NX_PTP_MASTER_TIME_SRC_INTERNAL_OSCILLATOR);
        if (status != NX_SUCCESS) return status;
    }

    /* Set port EUI */
    write_port_identity_eui(port_identity);

    /* Start the PTP clients */
    for (port_index_t i = 0; i < NUM_PHYS; i++) {

        /* Set port number */
        write_port_identity_number(port_identity, i);

        status = nx_ptp_client_start(
            &ptp_client[i],
            port_identity,
            NX_PTP_CLOCK_PORT_IDENTITY_SIZE,
            PTP_DOMAIN,
            NX_PTP_TRANSPORT_SPECIFIC_802,
            &ptp_event_callback,
            NX_NULL);
        if (status != NX_SUCCESS) return status;
    }

    return status;
}


/* This Thread starts the PTP client, and prints status information */
void ptp_event_thread_entry(uint32_t initial_input) {

    nx_status_t nx_status = NX_SUCCESS;
    tx_status_t tx_status = TX_SUCCESS;

    NX_PTP_TIME      time;
    NX_PTP_DATE_TIME date;

    uint32_t current_time    = tx_time_get();
    uint32_t next_print_time = current_time;
    uint32_t wait_ticks;

    bool             queue_empty;
    ptp_event_info_t event_info;

    uint8_t port_identity[NX_PTP_CLOCK_PORT_IDENTITY_SIZE]; /* 64-bit EUI + port */

    NX_PTP_CLIENT_MASTER *master;

    static_assert(PTP_VLAN == 0, "PTP Currently only supported on VLAN 0");

    /* Create the PTP client */
    for (phy_index_t i = 0; i < NUM_PHYS; i++) {
        nx_status = nx_ptp_client_create(
            &ptp_client[i],
            &nx_ip_instance,
            0,
            &nx_small_packet_pool,
            NX_INTERNAL_PTP_EVENT_THREAD_PRIORITY,
            (UCHAR *) nx_internal_ptp_stack[i],
            sizeof(nx_internal_ptp_stack[i]),
            &ptp_clock_callback,
            (void *) i);
        if (nx_status != NX_SUCCESS) error_handler();
    }

    /* Enable master mode initially on all ports with the lowest priority so
     * it is only used as a last restort. */
    nx_status = ptp_reset_all_clients();
    if (nx_status != NX_SUCCESS) error_handler();

    while (1) {

        /* Receive events from the event queue */
        do {

            /* Calculate timeout */
            current_time = tx_time_get();
            if ((int32_t) (current_time - next_print_time) >= 0) {
                wait_ticks = 0;
            } else {
                wait_ticks = next_print_time - current_time;
            }

            /* Receive event */
            tx_status = tx_queue_receive(&ptp_event_queue_handle, &event_info, wait_ticks);
            if (tx_status == NX_SUCCESS) {

                switch (event_info.event) {

                    case PTP_CLIENT_EVENT_MASTER: {

                        master = event_info.master;
                        if (master == NULL) error_handler();

                        /* If the new master is worse (+ve) or the same (0) as the current one then skip.
                         * This is to prevent the scenario where a port declares it is the master,
                         * but a different port has a better or the same clock connected to it. */
                        if ((connected_to_master != NULL) &&
                            nx_ptp_client_master_clock_compare(&connected_to_master->ptp_master, master) >= 0) break;

                        /* Save the new master */
                        connected_to_master = &ptp_client[event_info.port];
                        ptp_event_counters.new_master++;
                        memcpy(grandmaster_identity, master->nx_ptp_client_master_grandmaster_identity, NX_PTP_CLOCK_PORT_IDENTITY_SIZE);

                        /* Set port EUI */
                        write_port_identity_eui(port_identity);

                        /* Propagate master info to other clients and restart them */
                        for (port_index_t i = 0; i < NUM_PHYS; i++) {

                            /* Skip the newly connected to master port */
                            if (i == event_info.port) continue;

                            /* Stop the client */
                            nx_status = nx_ptp_client_stop(&ptp_client[i]);
                            if (nx_status != NX_SUCCESS) error_handler();

                            /* Apply the new master config */
                            nx_status = nx_ptp_client_master_enable(
                                &ptp_client[i],
                                NX_PTP_CLIENT_ROLE_SLAVE_AND_MASTER,
                                master->nx_ptp_client_master_priority1,
                                master->nx_ptp_client_master_priority2,
                                master->nx_ptp_client_master_clock_class,
                                master->nx_ptp_client_master_clock_accuracy,
                                master->nx_ptp_client_master_offset_scaled_log_variance,
                                master->nx_ptp_client_master_steps_removed + 1,
                                NX_PTP_MASTER_TIME_SRC_PTP);
                            if (nx_status != NX_SUCCESS) error_handler();

                            /* Inject the grandmaster's identity */
                            ptp_client[i].ptp_master.nx_ptp_client_master_grandmaster_identity = grandmaster_identity;

                            /* Start the client */
                            write_port_identity_number(port_identity, i);
                            nx_status = nx_ptp_client_start(
                                &ptp_client[i],
                                port_identity,
                                NX_PTP_CLOCK_PORT_IDENTITY_SIZE,
                                PTP_DOMAIN,
                                NX_PTP_TRANSPORT_SPECIFIC_802,
                                &ptp_event_callback,
                                NX_NULL);
                            if (nx_status != NX_SUCCESS) error_handler();
                        }

                        LOG_INFO("PTP: New master clock on port %d", event_info.port);
                        break;
                    }

                    case PTP_CLIENT_EVENT_SYNC: {
                        ptp_event_counters.sync++;
                        nx_ptp_client_sync_info_get(event_info.sync, NX_NULL, &ptp_utc_offset);
                        LOG_INFO("PTP: SYNC event, utc offset=%d", ptp_utc_offset);
                        break;
                    }

                    case PTP_CLIENT_EVENT_TIMEOUT: {

#if DEBUG
                        /* Only the port connected to the master should be able to time out */
                        assert(&ptp_client[event_info.port] == connected_to_master);
#endif

                        connected_to_master = NULL;
                        ptp_event_counters.master_timeout++;

                        /* Reset all clients to begin looking for a new master */
                        nx_status = ptp_reset_all_clients();
                        if (nx_status != NX_SUCCESS) error_handler();

                        LOG_INFO("PTP: Master clock TIMEOUT");
                        break;
                    }

                    default: {
                        LOG_WARNING("PTP: Unknown event");
                        error_handler();
                        break;
                    }
                }
                queue_empty = false;
            }

            else if (tx_status != TX_STATUS_QUEUE_EMPTY) {
                error_handler();
                queue_empty = false;
            }

            else {
                queue_empty = true;
            }
        } while (!queue_empty);

#if PTP_PRINT_TIME_INTERVAL

        /* Get, convert, and print the PTP time */
        current_time = tx_time_get();
        if ((int32_t) (current_time - next_print_time) >= 0) {

            /* We have gotten far behind so catch up */
            if ((current_time - next_print_time) > (PTP_PRINT_TIME_INTERVAL * 3)) {
                next_print_time = current_time;
            } else {
                next_print_time += PTP_PRINT_TIME_INTERVAL;
            }

            /* No master connected */
            // TODO: print local time instead
            if (connected_to_master == NULL) continue;
            continue; // TODO: enable when clock callback done

            /* Get and print the time */
            nx_status = nx_ptp_client_time_get(connected_to_master, &time);
            if (nx_status != NX_SUCCESS) {
                error_handler();
                continue;
            }
            nx_status = nx_ptp_client_utility_convert_time_to_date(&time, -ptp_utc_offset, &date);
            if (nx_status != NX_SUCCESS) {
                error_handler();
                continue;
            }
            LOG_INFO("PTP Time is %2u/%02u/%u %02u:%02u:%02u.%09lu\r\n",
                     date.day, date.month, date.year,
                     date.hour, date.minute, date.second, date.nanosecond);
        }

#endif
    }
}
