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
#include "nx_app.h"
#include "utils.h"
#include "ptp_init.h"
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


nx_status_t ptp_configure() {

    nx_status_t status = NX_SUCCESS;

    static_assert(PTP_CLK_FREQ <= 100000000, "PTP_CLK_FREQ too high");
    static_assert(PTP_COUNTER_ADDEND <= (1 << 31), "PTP_COUNTER_INCREMENT too small");

    /* Configure the ethernet timestamping register for PTP */
    ETH_PTP_ConfigTypeDef ptp_config;

    /* Get the current config */
    if (HAL_ETH_PTP_GetConfig(&heth, &ptp_config) != HAL_OK) status = NX_PTP_PARAM_ERROR;
    if (status != NX_SUCCESS) return status;

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
    if (HAL_ETH_PTP_SetConfig(&heth, &ptp_config) != HAL_OK) status = NX_STATUS_OPTION_ERROR;
    if (status != NX_SUCCESS) return status;

    return status;
}


/* Set MAC ingress correction register */
void ptp_set_ingress_correction() {

    uint32_t correction;

    correction  = (switch_handles[0].config->ports[SW0_PORT_HOST].speed == SJA1105_SPEED_100M) ? 120 : 800;
    correction += 2 * HZ_TO_NS(PTP_CLK_FREQ);             /* 2 * PTP_CLK_PER */
    correction  = (1 << 31) | (HZ_TO_NS(1) - correction); /* Ingress correction is subtracted so convert to two's complement */

    WRITE_REG(heth.Instance->MACTSICNR, correction);
}


/* Set MAC egress correction register */
void ptp_set_egress_correction() {

    uint32_t correction;

    correction  = (switch_handles[0].config->ports[SW0_PORT_HOST].speed == SJA1105_SPEED_100M) ? 40 : 400;
    correction += (1 * HZ_TO_NS(PTP_CLK_FREQ)) + (4 * 20); /* 1 * PTP_CLK_PER + 4 * TX_CLK_PER (Note: RMII TX_CLK is always 50MHz (20ns)) */

    WRITE_REG(heth.Instance->MACTSECNR, correction);
}
