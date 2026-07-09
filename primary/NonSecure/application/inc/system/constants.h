/*
 * constants.h
 *
 *  Created on: Jun 6, 2026
 *      Author: bens1
 */

#ifndef INC_CONSTANTS_H_
#define INC_CONSTANTS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "config.h"


/* ---------------------------------------------------------------------------- */
/* Linker Symbols */
/* ---------------------------------------------------------------------------- */

extern uint32_t __LOG_START__;
extern uint32_t __LOG_SIZE__;
extern uint32_t __TRACE_START__;
extern uint32_t __TRACE_SIZE__;

#define LOG_BASE           ((uint32_t) &__LOG_START__)
#define LOG_BUFFER_SIZE    ((uint32_t) &__LOG_SIZE__)
#define TRACE_BUFFER_START ((void *) &__TRACE_START__)
#define TRACE_BUFFER_SIZE  ((uint32_t) &__TRACE_SIZE__)

/* ---------------------------------------------------------------------------- */
/* Networking */
/* ---------------------------------------------------------------------------- */

#define MAC_ADDR_SIZE                 (6) /* 6 bytes */

#define NX_APP_SMALL_PACKET_POOL_SIZE ((SMALL_PACKET_SIZE + sizeof(NX_PACKET)) * NUM_SMALL_PACKETS)
#define NX_APP_BIG_PACKET_POOL_SIZE   ((BIG_PACKET_SIZE + sizeof(NX_PACKET)) * NUM_BIG_PACKETS)

#define PRIMARY_INTERFACE             (0) /* Primary NetXduo interface (0 = first normal interface, 1 = loopback) */

/* ---------------------------------------------------------------------------- */
/* PTP */
/* ---------------------------------------------------------------------------- */

/* PTP_COUNTER_INCREMENT is the resolution of the PTP clock. TimestampRolloverMode = 1 so the timer value
 * is 1:1 with ns. Using TimestampRolloverMode = 0 would improve the resolution from 1ns to 0.465ns, but
 * this is enough for most applications and simplifies timer reading and writing.
 *
 * Note: An increment of 10ns with a 100MHz PTP_CLK_FREQ would cause addend to overflow. To prevent this,
 *       and to leave headroom for fine adjustment the increment is set to 20ns. There is therefore no
 *       advantage to using a 100MHz clock over a 50MHz one.
 */
#define PTP_CLK_FREQ                    (100000000) /* Frequency of clk_ptp_ref_i (fed by PLL1Q's output) */
#define PTP_COUNTER_INCREMENT           (20)        /* 20ns */
#define PTP_COUNTER_ADDEND_DEFAULT      ((((uint64_t) 1 << 32) * (uint64_t) HZ_TO_NS(1)) / ((uint64_t) PTP_COUNTER_INCREMENT * (uint64_t) PTP_CLK_FREQ))

#define PTP_CLOCK_CONTROLLER_OUTPUT_MAX ((PTP_CLOCK_CONTROLLER_KP * MS_TO_NS(PTP_CLOCK_FINE_ADJUST_THRESHOLD)) + \
                                         (PTP_CLOCK_CONTROLLER_KI * PTP_CLOCK_CONTROLLER_INTEGRAL_MAX))
#if FEAT_PTP_SWITCH_SYNC
#define PTP_SWITCH_SYNC_CONTROLLER_OUTPUT_MAX ((PTP_SWITCH_SYNC_CONTROLLER_KP * S_TO_NS(1)) +                             \
                                               (PTP_SWITCH_SYNC_CONTROLLER_KI * PTP_SWITCH_SYNC_CONTROLLER_INTEGRAL_MAX))
#endif
#define PTP_MAC_SYNC_CONTROLLER_OUTPUT_MAX ((PTP_MAC_SYNC_CONTROLLER_KP * MS_TO_NS(PTP_MAC_SYNC_FINE_ADJUST_THRESHOLD)) + \
                                            (PTP_MAC_SYNC_CONTROLLER_KI * PTP_MAC_SYNC_CONTROLLER_INTEGRAL_MAX))

/* ---------------------------------------------------------------------------- */
/* Switch */
/* ---------------------------------------------------------------------------- */

#if HW_VERSION == 4
#define NUM_SWITCHES (1)
#elif HW_VERSION == 5
#define NUM_SWITCHES (2)
#else
#error "Unknown hardware version"
#endif

/* ---------------------------------------------------------------------------- */
/* PHY */
/* ---------------------------------------------------------------------------- */

#if HW_VERSION == 4
#define NUM_PHYS (4)
#elif HW_VERSION == 5
#define NUM_PHYS (7)
#else
#error "Unknown hardware version"
#endif

#define NUM_PORTS             (NUM_PHYS + 1)                                     /* Count the host as a port */

#define PHY_POLL_PERIOD       ((PHY_WAITING_FOR_LINK_TIME) + PHY_SLEEP_INTERVAL) /* The time spent for a full loop of waiting for link -> sleep -> waiting for link */
#define PHY_POLL_STAGGER_TIME (MIN(PHY_POLL_PERIOD / NUM_PHYS, PHY_WAITING_FOR_LINK_TIME))

/* ---------------------------------------------------------------------------- */
/* User storage config */
/* ---------------------------------------------------------------------------- */

#define USER_STORAGE_DHCP_VALID_ADDR  (0)
#define USER_STORAGE_DHCP_VALID_SIZE  (sizeof(bool))

#define USER_STORAGE_DHCP_RECORD_ADDR (USER_STORAGE_DHCP_VALID_ADDR + USER_STORAGE_DHCP_VALID_SIZE)
#define USER_STORAGE_DHCP_RECORD_SIZE (sizeof(NX_DHCP_CLIENT_RECORD))

#define USER_STORAGE_TEST_ADDR        (USER_STORAGE_DHCP_RECORD_ADDR + USER_STORAGE_DHCP_RECORD_SIZE)
#define USER_STORAGE_TEST_SIZE        (sizeof(uint32_t))


#ifdef __cplusplus
}
#endif

#endif /* INC_CONSTANTS_H_ */
