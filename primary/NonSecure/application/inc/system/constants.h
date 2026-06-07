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

#define PTP_CLOCK_CONTROLLER_OUTPUT_MAX ((PTP_CLOCK_CONTROLLER_KP * S_TO_NS(1)) + (PTP_CLOCK_CONTROLLER_KI * PTP_CLOCK_CONTROLLER_INTEGRAL_MAX))

/* ---------------------------------------------------------------------------- */
/* Switch */
/* ---------------------------------------------------------------------------- */

#define NUM_SWITCHES ((HW_VERSION == 4) ? 1 : 2)

/* ---------------------------------------------------------------------------- */
/* PHY */
/* ---------------------------------------------------------------------------- */

#define NUM_PHYS              ((HW_VERSION == 4) ? 4 : 7)
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
