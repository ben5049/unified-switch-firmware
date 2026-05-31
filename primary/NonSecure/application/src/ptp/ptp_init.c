/*
 * ptp_init.c
 *
 *  Created on: May 15, 2026
 *      Author: bens1
 */

#include "stdint.h"
#include "assert.h"
#include "hal.h"
#include "eth.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "utils.h"
#include "ptp_init.h"
#include "ptp_thread.h"
#include "ptp_utils.h"
#include "switch_thread.h"
#include "switch_utils.h"


/* PTP_COUNTER_INCREMENT is the resolution of the PTP clock. TimestampRolloverMode = 1 so the timer value
 * is 1:1 with ns. Using TimestampRolloverMode = 0 would improve the resolution from 1ns to 0.465ns, but
 * this is enough for most applications and simplifies timer reading and writing.
 *
 *
 * Note: An increment of 10ns with a 100MHz PTP_CLK_FREQ would cause addend to overflow. To prevent this,
 *       and to leave headroom for fine adjustment the increment is set to 20ns. There is therefore no
 *       advantage to using a 100MHz clock over a 50MHz one.
 */

#define PTP_COUNTER_INCREMENT (20) /* 20ns */
#define PTP_COUNTER_ADDEND    (((uint64_t) 1 << 32) * (uint64_t) HZ_TO_NS(1)) / ((uint64_t) PTP_COUNTER_INCREMENT * (uint64_t) PTP_CLK_FREQ)


volatile uint32_t srcmeta_msw = 0;
volatile uint32_t srcmeta_lsw = 0;


/* Set MAC ingress correction register */
static void ptp_set_ingress_correction() {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         correction;
    uint16_t         speed;

    status = switch_get_speed(PORT_HOST, &speed);
    SWITCH_CHECK(status);

    correction  = (speed == 100) ? 120 : 800;
    correction += 2 * HZ_TO_NS(PTP_CLK_FREQ);             /* 2 * PTP_CLK_PER */
    correction  = (1 << 31) | (HZ_TO_NS(1) - correction); /* Ingress correction is subtracted so convert to two's complement */

    WRITE_REG(heth.Instance->MACTSICNR, correction);
}


/* Set MAC egress correction register */
static void ptp_set_egress_correction() {

    sja1105_status_t status = SJA1105_OK;
    uint32_t         correction;
    uint16_t         speed;

    status = switch_get_speed(PORT_HOST, &speed);
    SWITCH_CHECK(status);

    correction  = (speed == 100) ? 40 : 400;
    correction += (1 * HZ_TO_NS(PTP_CLK_FREQ)) + (4 * 20); /* 1 * PTP_CLK_PER + 4 * TX_CLK_PER (Note: RMII TX_CLK is always 50MHz (20ns)) */

    WRITE_REG(heth.Instance->MACTSECNR, correction);
}


static hal_status_t ptp_configure() {

    hal_status_t status = HAL_OK;

    static_assert(PTP_CLK_FREQ <= 100000000, "PTP_CLK_FREQ too high");
    static_assert(PTP_COUNTER_ADDEND <= (1 << 31), "PTP_COUNTER_INCREMENT too small");

    /* Configure the ethernet timestamping register for PTP */
    ETH_PTP_ConfigTypeDef ptp_config;

    /* Get the current config */
    status = HAL_ETH_PTP_GetConfig(&heth, &ptp_config);
    if (status != HAL_OK) return status;

    /* Update the config */
    ptp_config.TimestampUpdateMode   = ENABLE; /* Fine mode */
    ptp_config.TimestampAddend       = PTP_COUNTER_ADDEND;
    ptp_config.TimestampAddendUpdate = ENABLE;
    ptp_config.TimestampSubsecondInc = (PTP_COUNTER_INCREMENT & 0xff) << 16; /* For a 50MHz PTP clock increment by 20ns each time (RM0481 page 2935)*/

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

    /* Take timestamps for event messages relevant to gPTP (SYNC, Pdelay_Req, Pdelay_Resp) (RM0481 page 2728) */
#ifdef NX_ENABLE_GPTP
    ptp_config.TimestampSnapshots = 0b01;
    ptp_config.TimestampMaster    = DISABLE;
    ptp_config.TimestampEvent     = ENABLE;
#else
    ptp_config.TimestampSnapshots = 0b00;
    ptp_config.TimestampMaster    = DISABLE; /* Don't care */
    ptp_config.TimestampEvent     = DISABLE;
#endif

    ptp_config.TimestampFilter     = DISABLE; /* Don't bother filtering by destination address */
    ptp_config.TimestampStatusMode = DISABLE; /* Don't overwrite transmit timestamps */

    /* Write the new config */
    status = HAL_ETH_PTP_SetConfig(&heth, &ptp_config);
    if (status != HAL_OK) return status;

    /* Set ingresss and egress corrections */
    ptp_set_ingress_correction();
    ptp_set_egress_correction();

    return status;
}


tx_status_t ptp_start() {

    tx_status_t  status     = TX_SUCCESS;
    hal_status_t hal_status = HAL_OK;
    uint32_t     flags;
    bool         trapped;
    bool         send_meta;
    bool         incl_srcpt;
    uint32_t     msw, lsw;

    for (switch_index_t i = SWITCH0; i < NUM_SWITCHES; i++) {

        /* Check all switches trap PTP frames */
        if (SJA1105_MACAddrTrapTest(&switch_handles[i], ptp_dst_addr, &trapped, &send_meta, &incl_srcpt) != SJA1105_OK) return TX_ERROR;
        assert(trapped);
        assert(send_meta);
        assert(incl_srcpt);

        /* Cache the MAC address used as a source by META frames */
        if (SJA1105_GetSRCMETA(&switch_handles[i], &msw, &lsw) != SJA1105_OK) return TX_ERROR;
        if (i == 0) {
            srcmeta_msw = msw;
            srcmeta_lsw = lsw;
        } else {
            assert(msw == srcmeta_msw);
            assert(lsw == srcmeta_lsw);
        }
    }

    /* Configure MAC PTP registers */
    hal_status = ptp_configure();
    HAL_CHECK(hal_status);

    /* Flush the queues. Queues containing packets must have all their packets released */
    status = ptp_flush_packet_queue(&ptp_tx_queue_handle);
    if (status != TX_SUCCESS) return status;
    status = ptp_flush_packet_queue(&ptp_rx_packet_queue_handle);
    if (status != TX_SUCCESS) return status;
    status = ptp_flush_packet_queue(&ptp_rx_meta_queue_handle);
    if (status != TX_SUCCESS) return status;
    status = tx_queue_flush(&ptp_event_queue_handle);
    if (status != TX_SUCCESS) return status;
    status = tx_queue_flush(&ptp_mac_sync_queue_handle);
    if (status != TX_SUCCESS) return status;

    /* Clear event flags */
    status = tx_event_flags_get(&ptp_tx_events_handle, PTP_EVENT_ALL, TX_OR_CLEAR, &flags, TX_NO_WAIT);
    if ((status != TX_SUCCESS) && (status != TX_NO_EVENTS)) return status;
    status = tx_event_flags_get(&ptp_mac_sync_events_handle, PTP_EVENT_ALL, TX_OR_CLEAR, &flags, TX_NO_WAIT);
    if ((status != TX_SUCCESS) && (status != TX_NO_EVENTS)) return status;
    status = tx_event_flags_get(&ptp_switch_sync_events_handle, PTP_EVENT_ALL, TX_OR_CLEAR, &flags, TX_NO_WAIT);
    if ((status != TX_SUCCESS) && (status != TX_NO_EVENTS)) return status;

    /* Start the TX thread before the PTP clients to avoid queues building up */
    status = tx_thread_resume(&ptp_tx_thread_handle);
    if (status != TX_SUCCESS) return status;

    /* Start the event thread. This also starts the PTP clients */
    status = tx_thread_resume(&ptp_event_thread_handle);
    if (status != TX_SUCCESS) return status;

    /* Start the RX thread after the PTP clients to avoid giving packets from before the PTP clients are started */
    status = tx_thread_resume(&ptp_rx_thread_handle);
    if (status != TX_SUCCESS) return status;

    /* Start the sync threads last since they rely on both TX and RX threads */
    status = tx_thread_resume(&ptp_mac_sync_thread_handle);
    if (status != TX_SUCCESS) return status;
    status = tx_thread_resume(&ptp_switch_sync_thread_handle);
    if (status != TX_SUCCESS) return status;

    return status;
}


tx_status_t ptp_stop() {

    tx_status_t status = TX_NOT_DONE;

    // TODO: stop threads

    /* Stop timers */
    status = tx_timer_deactivate(&ptp_mac_sync_timer);
    TX_CHECK(status);
#if NUM_SWITCHES > 1
    status = tx_timer_deactivate(&ptp_switch_sync_timer);
    TX_CHECK(status);
#endif

    return TX_NOT_DONE;
}
