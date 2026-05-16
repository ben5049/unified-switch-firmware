/*
 * ptp_thread.c
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

NX_PTP_CLIENT  ptp_client __attribute__((section(".ETH")));
static uint8_t nx_internal_ptp_stack[NX_INTERNAL_PTP_THREAD_STACK_SIZE] __attribute__((section(".ETH")));

TX_THREAD ptp_thread_handle;
uint8_t   ptp_thread_stack[PTP_THREAD_STACK_SIZE];

TX_QUEUE ptp_tx_queue_handle; /* Stores callback events */
uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

ptp_event_counters_t ptp_event_counters;


/* Event callback for NetX PTP client */
static UINT ptp_event_callback(NX_PTP_CLIENT *ptp_client_ptr, UINT event, VOID *event_data, VOID *callback_data) {
    NX_PARAMETER_NOT_USED(callback_data);

    ptp_event_t ptp_event = {.event = event, .event_data = event_data};

    return tx_queue_send(&ptp_tx_queue_handle, &ptp_event, TX_NO_WAIT);
}


/* This Thread starts the PTP client, and prints status information */
void ptp_thread_entry(uint32_t initial_input) {

    nx_status_t nx_status = NX_SUCCESS;
    tx_status_t tx_status = TX_SUCCESS;

    NX_PTP_TIME      time;
    NX_PTP_DATE_TIME date;

    uint32_t next_print_time = 0;
    uint32_t current_time    = 0;

    ptp_event_t event;

    /* Configure the MAC PTP control registers */
    nx_status = ptp_configure();
    if (nx_status != NX_SUCCESS) error_handler();

    /* Configure the MAC timestamp correction registers */
    ptp_set_ingress_correction();
    ptp_set_egress_correction();

    // TODO: enable PTP offload?

    /* Create the PTP client */
    nx_status = nx_ptp_client_create(
        &ptp_client,
        &nx_ip_instance,
        0,
        &nx_packet_pool,
        NX_INTERNAL_PTP_THREAD_PRIORITY,
        (UCHAR *) nx_internal_ptp_stack,
        sizeof(nx_internal_ptp_stack),
        &nx_driver_ptp_clock_callback,
        NX_NULL);
    if (nx_status != NX_SUCCESS) error_handler();

    /* Start the PTP client */
    nx_status = nx_ptp_client_start(
        &ptp_client,
        NX_NULL, /* Auto-generated ID */
        0,
        PTP_DOMAIN,
#ifdef NX_ENABLE_GPTP
        1,
#else
        0,
#endif
        &ptp_event_callback,
        NX_NULL);
    if (nx_status != NX_SUCCESS) error_handler();

#ifdef NX_PTP_ENABLE_MASTER
    nx_status = nx_ptp_client_master_enable(
        &ptp_client,
        NX_PTP_CLIENT_ROLE_SLAVE_AND_MASTER,
        NX_PTP_CLIENT_MASTER_PRIORITY,
        PTP_CLIENT_MASTER_SUB_PRIORITY,
        NX_PTP_CLIENT_MASTER_CLOCK_CLASS,
        NX_PTP_CLIENT_MASTER_ACCURACY,
        NX_PTP_CLIENT_MASTER_CLOCK_VARIANCE,
        NX_PTP_CLIENT_MASTER_CLOCK_STEPS_REMOVED,
        NX_NULL); /* Enable master mode with the lowest priority so it is only used as a last restort. TODO: Randomise or make different */
    if (nx_status != NX_SUCCESS) error_handler();
#endif

    while (1) {

        /* Receive events from the NetX PTP thread */
        tx_status = tx_queue_receive(&ptp_tx_queue_handle, &event, CONSTRAIN(PTP_PRINT_TIME_INTERVAL, MS_TO_TICKS(100), TX_WAIT_FOREVER));
        if (tx_status == NX_SUCCESS) {

            switch (event.event) {
                case NX_PTP_CLIENT_EVENT_MASTER: {
                    ptp_event_counters.new_master++;
                    LOG_INFO("PTP: New master clock");
                    break;
                }

                case NX_PTP_CLIENT_EVENT_SYNC: {
                    ptp_event_counters.sync++;
                    nx_ptp_client_sync_info_get((NX_PTP_CLIENT_SYNC *) event.event_data, NX_NULL, &ptp_utc_offset);
                    LOG_INFO("PTP: SYNC event, utc offset=%d", ptp_utc_offset);
                    break;
                }

                case NX_PTP_CLIENT_EVENT_TIMEOUT: {
                    ptp_event_counters.master_timeout++;
                    LOG_INFO("PTP: Master clock TIMEOUT");
                    break;
                }

                default: {
                    LOG_WARNING("PTP: Unknown event");
                    break;
                }
            }
        }

        else if (tx_status != TX_STATUS_QUEUE_EMPTY) {
            error_handler();
        }

#if PTP_PRINT_TIME_INTERVAL

        /* Get, convert, and print the PTP time */
        current_time = tx_time_get_ms();
        if (current_time >= next_print_time) {
            nx_status = nx_ptp_client_time_get(&ptp_client, &time);
            if (nx_status != NX_SUCCESS) error_handler();
            nx_status = nx_ptp_client_utility_convert_time_to_date(&time, -ptp_utc_offset, &date);
            if (nx_status != NX_SUCCESS) error_handler();
            LOG_INFO("PTP Time is %2u/%02u/%u %02u:%02u:%02u.%09lu\r\n", date.day, date.month, date.year, date.hour, date.minute, date.second, date.nanosecond);
            next_print_time = current_time + PTP_PRINT_TIME_INTERVAL;
        }

#endif
    }
}
