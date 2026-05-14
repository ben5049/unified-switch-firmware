/*
 * ptp_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"

#include "hal.h"
#include "eth.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"
#include "nx_stm32_eth_driver.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "ptp_callbacks.h"
#include "utils.h"


SHORT ptp_utc_offset = 0;

NX_PTP_CLIENT  ptp_client __attribute__((section(".ETH")));
static uint8_t nx_internal_ptp_stack[NX_INTERNAL_PTP_THREAD_STACK_SIZE] __attribute__((section(".ETH")));

TX_THREAD ptp_thread_handle;
uint8_t   ptp_thread_stack[PTP_THREAD_STACK_SIZE];

TX_QUEUE ptp_tx_queue_handle; /* Stores callback events */
uint32_t ptp_tx_queue_stack[PTP_TX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];

ptp_event_counters_t ptp_event_counters;


/* This Thread starts the PTP client, processes transmitted timestamps, and prints nx_status information */
void ptp_thread_entry(uint32_t initial_input) {

    nx_status_t nx_status = NX_SUCCESS;
    tx_status_t tx_status = TX_SUCCESS;

    NX_PTP_TIME      time;
    NX_PTP_DATE_TIME date;

    uint32_t next_print_time = 0;
    uint32_t current_time    = 0;

    ptp_event_t event;


    /* Increment is the period of the PTP clock. TimestampRolloverMode = 1 so the timer value is 1:1 with ns.
     * Using TimestampRolloverMode = 0 would improve the resolution from 1ns to 0.465ns, but this is enough
     * for most applications and simplifies timer reading and writing.
     */
    uint32_t increment = 20;                                                                                             /* 20ns */
    uint32_t addend    = (((uint64_t) 1 << 32) * (uint64_t) 1000000000) / ((uint64_t) increment * (uint64_t) 100000000); /* 0x33333333 for 250MHx HCLK*/

    /* Configure the ethernet timestamping register for PTP */
    ETH_PTP_ConfigTypeDef ptp_config;

    /* Get the current config */
    if (HAL_ETH_PTP_GetConfig(&heth, &ptp_config) != HAL_OK) nx_status = NX_PTP_PARAM_ERROR;
    if (nx_status != NX_STATUS_SUCCESS) error_handler();

    /* Update the config */
    ptp_config.TimestampUpdateMode   = ENABLE; /* Fine mode */
    ptp_config.TimestampAddend       = addend;
    ptp_config.TimestampAddendUpdate = ENABLE;

    ptp_config.TimestampAll          = DISABLE;
    ptp_config.TimestampRolloverMode = ENABLE; /* Every 1,000,000,000 nanoseconds the seconds count is incremented */
    ptp_config.TimestampV2           = ENABLE; /* IEE 1588-2008 (PTPv2) enabled */

#ifdef NX_ENABLE_GPTP
    ptp_config.TimestampEthernet = ENABLE;  /* Enable processing of PTP frames embedded directly in ethernet packets */
    ptp_config.TimestampIPv6     = DISABLE; /* Disable processing of PTP frames embedded in IPv6-UDP packets */
    ptp_config.TimestampIPv4     = DISABLE; /* Disable processing of PTP frames embedded in IPv4-UDP packets */
#else
    ptp_config.TimestampEthernet = DISABLE; /* Disable processing of PTP frames embedded directly in ethernet packets */
    ptp_config.TimestampIPv6     = ENABLE;  /* Enable processing of PTP frames embedded in IPv6-UDP packets */
    ptp_config.TimestampIPv4     = ENABLE;  /* Enable processing of PTP frames embedded in IPv4-UDP packets */
#endif

    ptp_config.TimestampEvent        = DISABLE;                  /* ┐ These settings mean timestamps are taken for SYNC, Follow_Up, Delay_Req, */
    ptp_config.TimestampMaster       = DISABLE;                  /* ├ Delay_Resp, Pdelay_Req, Pdelay_Resp, and Pdelay_Resp_Follow_Up messages. */
    ptp_config.TimestampSnapshots    = ENABLE;                   /* ┘ These are all the master and slave message types. */
    ptp_config.TimestampFilter       = DISABLE;                  /* Don't bother filtering by destination address */
    ptp_config.TimestampStatusMode   = DISABLE;                  /* Don't overwrite transmit timestamps */
    ptp_config.TimestampSubsecondInc = (increment & 0xff) << 16; /* For a 50MHz PTP clock increment by 20ns each time (RM0481 page 2935)*/

    /* Write the new config */
    if (HAL_ETH_PTP_SetConfig(&heth, &ptp_config) != HAL_OK) nx_status = NX_STATUS_OPTION_ERROR;
    if (nx_status != NX_STATUS_SUCCESS) error_handler();


    /* TODO: Set the ETH_MACTSECNR_TSEC and ETH_MACTSICNR_TSIC registers */


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
        NX_NULL,
        0,
        0,
        0,
        &ptp_event_callback,
        NX_NULL);
    if (nx_status != NX_SUCCESS) error_handler();

#ifdef NX_PTP_ENABLE_MASTER
    nx_status = nx_ptp_client_master_enable(&ptp_client, NX_PTP_CLIENT_ROLE_SLAVE_AND_MASTER, NX_PTP_CLIENT_MASTER_PRIORITY, PTP_CLIENT_MASTER_SUB_PRIORITY, NX_PTP_CLIENT_MASTER_CLOCK_CLASS, NX_PTP_CLIENT_MASTER_ACCURACY, NX_PTP_CLIENT_MASTER_CLOCK_VARIANCE, NX_PTP_CLIENT_MASTER_CLOCK_STEPS_REMOVED, NX_NULL); /* Enable master mode with the lowest priority so it is only used as a last restort. TODO: Randomise or make different */
    if (nx_status != NX_SUCCESS) error_handler();
#endif

    while (1) {

        tx_status = tx_queue_receive(&ptp_tx_queue_handle, &event, CONSTRAIN(PTP_PRINT_TIME_INTERVAL, MS_TO_TICKS(100), TX_WAIT_FOREVER));
        if (tx_status == NX_SUCCESS) {

            switch (event.event) {
                case NX_PTP_CLIENT_EVENT_MASTER: {
                    ptp_event_counters.new_master++;
                    LOG_INFO("PTP: new MASTER clock");
                    break;
                }

                case NX_PTP_CLIENT_EVENT_SYNC: {
                    ptp_event_counters.sync++;
                    nx_ptp_client_sync_info_get((NX_PTP_CLIENT_SYNC *) event.event_data, NX_NULL, &ptp_utc_offset);
                    LOG_INFO("PTP: SYNC event: utc offset=%d", ptp_utc_offset);
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


#if (PTP_PRINT_TIME_INTERVAL != UINT32_MAX)

        /* Get, convert, and print the PTP time (this ironically uses the non-precise threadx time to delay between prints) */
        current_time = tx_time_get_ms();
        if (current_time >= next_print_time) {
            nx_status = nx_ptp_client_time_get(&ptp_client, &time);
            if (nx_status != NX_SUCCESS) error_handler();
            nx_status = nx_ptp_client_utility_convert_time_to_date(&time, -ptp_utc_offset, &date);
            if (nx_status != NX_SUCCESS) error_handler();
            printf("%2u/%02u/%u %02u:%02u:%02u.%09lu\r\n", date.day, date.month, date.year, date.hour, date.minute, date.second, date.nanosecond);
            next_print_time = current_time + PTP_PRINT_TIME_INTERVAL;
        }

#endif /* (PTP_PRINT_TIME_INTERVAL != TX_WAIT_FOREVER) */
    }
}
