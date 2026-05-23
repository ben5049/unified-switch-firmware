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
#include "switch_thread.h"


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


const uint8_t ptp_dst_addr[MAC_ADDR_SIZE] = {
    (uint8_t) (PTP_ETHERNET_ADDR_MSB >> 8),  /* Index 0: 0x01 */
    (uint8_t) (PTP_ETHERNET_ADDR_MSB),       /* Index 1: 0x80 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 24), /* Index 2: 0xC2 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 16), /* Index 3: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 8),  /* Index 4: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB)        /* Index 5: 0x0E */
};


static tx_status_t ptp_configure() {

    tx_status_t status = TX_SUCCESS;

    static_assert(PTP_CLK_FREQ <= 100000000, "PTP_CLK_FREQ too high");
    static_assert(PTP_COUNTER_ADDEND <= (1 << 31), "PTP_COUNTER_INCREMENT too small");

    /* Configure the ethernet timestamping register for PTP */
    ETH_PTP_ConfigTypeDef ptp_config;

    /* Get the current config */
    if (HAL_ETH_PTP_GetConfig(&heth, &ptp_config) != HAL_OK) status = TX_STATUS_OPTION_ERROR;
    if (status != TX_SUCCESS) return status;

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
    if (HAL_ETH_PTP_SetConfig(&heth, &ptp_config) != HAL_OK) status = TX_STATUS_OPTION_ERROR;
    if (status != TX_SUCCESS) return status;

    return status;
}


/* Set MAC ingress correction register */
static void ptp_set_ingress_correction() {

    uint32_t correction;

    correction  = (switch_handles[0].config->ports[SW0_PORT_HOST].speed == SJA1105_SPEED_100M) ? 120 : 800;
    correction += 2 * HZ_TO_NS(PTP_CLK_FREQ);             /* 2 * PTP_CLK_PER */
    correction  = (1 << 31) | (HZ_TO_NS(1) - correction); /* Ingress correction is subtracted so convert to two's complement */

    WRITE_REG(heth.Instance->MACTSICNR, correction);
}


/* Set MAC egress correction register */
static void ptp_set_egress_correction() {

    uint32_t correction;

    correction  = (switch_handles[0].config->ports[SW0_PORT_HOST].speed == SJA1105_SPEED_100M) ? 40 : 400;
    correction += (1 * HZ_TO_NS(PTP_CLK_FREQ)) + (4 * 20); /* 1 * PTP_CLK_PER + 4 * TX_CLK_PER (Note: RMII TX_CLK is always 50MHz (20ns)) */

    WRITE_REG(heth.Instance->MACTSECNR, correction);
}


tx_status_t ptp_start() {

    tx_status_t status = TX_SUCCESS;
    uint32_t    flags;
    bool        trapped;
    bool        send_meta;
    bool        incl_srcpt;

    /* Check all switches trap PTP frames */
    for (uint_fast8_t i = 0; i < NUM_SWITCHES; i++) {
        if (SJA1105_MACAddrTrapTest(&switch_handles[i], ptp_dst_addr, &trapped, &send_meta, &incl_srcpt) != SJA1105_OK) error_handler();
        assert(trapped);
        assert(send_meta);
        assert(incl_srcpt);
    }

    /* Configure the MAC PTP control registers */
    status = ptp_configure();
    if (status != TX_SUCCESS) return status;

    /* Configure the MAC timestamp correction registers */
    ptp_set_ingress_correction();
    ptp_set_egress_correction();

    /* Flush the queues. Queues containing packets must have all their packets released */
    ptp_flush_packet_queue(&ptp_tx_queue_handle);
    ptp_flush_packet_queue(&ptp_rx_packet_queue_handle);
    ptp_flush_packet_queue(&ptp_rx_meta_queue_handle);
    status = tx_queue_flush(&ptp_event_queue_handle);
    if (status != TX_SUCCESS) return status;

    /* Clear event flags */
    status = tx_event_flags_get(&ptp_tx_events_handle, PTP_TX_EVENT_ALL, TX_OR_CLEAR, &flags, TX_NO_WAIT);
    if (status != TX_SUCCESS) return status;

    /* Start the TX thread before the PTP clients to avoid queues building up */
    status = tx_thread_resume(&ptp_tx_thread_handle);
    if (status != TX_SUCCESS) return status;

    /* Start the event thread. This also starts the PTP clients */
    status = tx_thread_resume(&ptp_event_thread_handle);
    if (status != TX_SUCCESS) return status;

    /* Start the RX thread after the PTP clients to avoid giving packets from before the PTP clients are started */
    status = tx_thread_resume(&ptp_rx_thread_handle);
    if (status != TX_SUCCESS) return status;

    return status;
}


tx_status_t ptp_stop() {
    // TODO: stop threads, clear queues etc
    return -1;
}


/* Drain all items from a PTP queue and release any packets */
void ptp_flush_packet_queue(TX_QUEUE *queue_ptr) {

    ptp_event_info_t event_info;
    NX_PACKET       *packet;

    /* Loop until tx_queue_receive returns TX_QUEUE_EMPTY (or another error) */
    while (tx_queue_receive(queue_ptr, &event_info, TX_NO_WAIT) == TX_SUCCESS) {

        /* Ensure the data pointer actually contains a packet before releasing */
        switch (event_info.event) {

            case PTP_TX_EVENT_SEND_PACKET:
            case PTP_RX_EVENT_RECEIVE_PACKET:
                packet = (NX_PACKET *) event_info.data;
                if (nx_packet_release(packet) != NX_SUCCESS) error_handler();
                break;

            default:
                break;
        }
    }
}
