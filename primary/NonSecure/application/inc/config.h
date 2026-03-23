/*
 * config.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_api.h"
#include "zenoh_generic_platform.h"


/* ---------------------------------------------------------------------------- */
/* Thread Enables */
/* ---------------------------------------------------------------------------- */

#define ENABLE_STP_THREAD false /* More important TODO: replace with custom implementation, MSTP is too heavy. TODO: fix this thread. Each time it calls for a flush the 50MHz REF_CLK is reset which is probably very bad */

/* ---------------------------------------------------------------------------- */
/* Common Config */
/* ---------------------------------------------------------------------------- */

#define DEFAULT_SECURE_STACK_SIZE (1024) /* Any thread that calls secure functions should allocate this ammount of secure stack */

/* ---------------------------------------------------------------------------- */
/* Logging Config */
/* ---------------------------------------------------------------------------- */

#define NUM_LOGGERS     (7)
#define LOG_BASE        (SRAM1_BASE_NS)
#define LOG_BUFFER_SIZE (128 * 1024) /* Same size in Secure */ // TODO: generate from linker
#define LOG_TIMEOUT     (100)                                  /* ms */

/* ---------------------------------------------------------------------------- */
/* State Machine Config */
/* ---------------------------------------------------------------------------- */

#define STATE_MACHINE_THREAD_STACK_SIZE         (1024)
#define STATE_MACHINE_THREAD_PRIORITY           (9)
#define STATE_MACHINE_THREAD_PREMPTION_PRIORITY (9)

/* ---------------------------------------------------------------------------- */
/* Networking Common Config */
/* ---------------------------------------------------------------------------- */

#define MAC_ADDR_OCTET1                  (0x02)
#define MAC_ADDR_OCTET2                  (0x00)
#define MAC_ADDR_OCTET3                  (0x00)
#define MAC_ADDR_OCTET4                  (0xd3)
#define MAC_ADDR_OCTET5                  (0x6a)
#define MAC_ADDR_OCTET6                  (0x48)

#define NX_APP_DEFAULT_TIMEOUT           (10000)                                           /* Generic timeout for nx events (e.g. TCP send) in ms */
#define NX_APP_PACKET_POOL_SIZE          ((DEFAULT_PAYLOAD_SIZE + sizeof(NX_PACKET)) * 32) /* Enough space for 32 max size packets */

#define NX_DEFAULT_IP_ADDRESS            (0)                                               /* TODO: Set this */
#define NX_DEFAULT_NET_MASK              (0)                                               /* TODO: Set this */
#define NX_INTERNAL_IP_THREAD_STACK_SIZE (2 * 1024)
#define NX_INTERNAL_IP_THREAD_PRIORITY   (NX_APP_THREAD_PRIORITY)

#define DEFAULT_PAYLOAD_SIZE             (1536) /* Ethernet payload size field (0x600) */

#define DEFAULT_ARP_CACHE_SIZE           (1024)

#define NX_APP_THREAD_STACK_SIZE         (2 * 1024)
#define NX_APP_THREAD_PRIORITY           (10)

#define NUM_VLANS                        (8) /* Currently only used for STP (unused due to RSTP not MSTP) */

#define PRIMARY_INTERFACE                (0) /* Primary NetXduo interface (0 = first normal interface, 1 = loopback) */

#if HW_VERSION == 4
#define PORT0_SPEED_MBPS (1000) /* 88Q2112 #1 (100 or 1000 Mbps) */
#define PORT1_SPEED_MBPS (1000) /* 88Q2112 #2 (100 or 1000 Mbps) */
#define PORT2_SPEED_MBPS (1000) /* 88Q2112 #3 (100 or 1000 Mbps) */
#define PORT3_SPEED_MBPS (10)   /* 10BASE-T1S (10 Mbps) */
#define PORT4_SPEED_MBPS (100)  /* Host (10 or 100 Mbps) */
#elif HW_VERSION == 5
#define PORT0_SPEED_MBPS (1000) /* DP83867 (10, 100 or 1000 Mbps) */
#define PORT1_SPEED_MBPS (1000) /* 88Q2112 #1 (100 or 1000 Mbps) */
#define PORT2_SPEED_MBPS (1000) /* 88Q2112 #2 (100 or 1000 Mbps) */
#define PORT3_SPEED_MBPS (1000) /* 88Q2112 #3 (100 or 1000 Mbps) */
#define PORT4_SPEED_MBPS (1000) /* 88Q2112 #4 (100 or 1000 Mbps) */
#define PORT5_SPEED_MBPS (1000) /* 88Q2112 #5 (100 or 1000 Mbps) */
#define PORT6_SPEED_MBPS (10)   /* 10BASE-T1S (10 Mbps) */
#define PORT7_SPEED_MBPS (100)  /* Host (10 or 100 Mbps) */
#endif

#define PHY_LINK_REQUIRED_FOR_NX_LINK (true)  /* Setting this to false means NetXduo will only require the switch to be initialed to count as having a link up. Default = true*/

#define DHCP_RECORD_SAVE_INTERVAL     (10000) /* How often to save the current DHCP record for restoration later in case of reboot in ms */

#define ENABLE_DHCP_RESTORE           (true)

/* ---------------------------------------------------------------------------- */
/* Link Config */
/* ---------------------------------------------------------------------------- */

#define NX_LINK_THREAD_STACK_SIZE                 (2 * 1024)
#define NX_LINK_THREAD_PRIORITY                   (9)

#define NX_APP_CABLE_CONNECTION_CHECK_UP_PERIOD   (300)  /* Interval between link checks in ms when the link is down */
#define NX_APP_CABLE_CONNECTION_CHECK_DOWN_PERIOD (1000) /* Interval between link checks in ms when the link is up */

/* ---------------------------------------------------------------------------- */
/* PTP Config */
/* ---------------------------------------------------------------------------- */

#define NX_INTERNAL_PTP_THREAD_STACK_SIZE (1024)
#define NX_INTERNAL_PTP_THREAD_PRIORITY   (2) /* This must be very high priority. Firstly to minimise delays, and secondly to prevent another thread prempting it and sending a packet that receives the timestamp meant for a PTP packet. */

#define PTP_THREAD_STACK_SIZE             (1024)
#define PTP_THREAD_PRIORITY               (4)
#define PTP_TX_QUEUE_SIZE                 (10)
#define PTP_PRINT_TIME_INTERVAL           (1000) /* Time interval between printing the PTP time in ms. Must be >= 100ms. Set to UINT32_MAX to disable printing */

#define PTP_CLIENT_MASTER_SUB_PRIORITY    (248)  /* The subpriority of this device for BMCA. Default for an end instance is 248 */

/* ---------------------------------------------------------------------------- */
/* Switch Config */
/* ---------------------------------------------------------------------------- */

#define SWITCH_TIMEOUT_MS                 (100)  /* Default timeout for switch operations in ms */
#define SWITCH_MANAGMENT_ROUTE_TIMEOUT_MS (1000) /* The time after allocating a management route when that route can be freed if not used */

#define SWITCH_THREAD_STACK_SIZE          (4 * 1024)
#define SWITCH_THREAD_PRIORITY            (15)
#define SWITCH_THREAD_PREMPTION_PRIORITY  (15)

#define SWITCH_MAINTENANCE_INTERVAL       (500)                     /* Time between performing switch maintenance operations in ms */
#define SWITCH_PUBLISH_STATS_INTERVAL     (1000)                    /* Time between publishing switch statistic in ms */

#define SWITCH_MEM_POOL_SIZE              (1024 * sizeof(uint32_t)) /* 1024 Words should be enough for most variable length tables. TODO: Check */

/* ---------------------------------------------------------------------------- */
/* PHY Config */
/* ---------------------------------------------------------------------------- */

#define NUM_PHYS                      ((HW_VERSION == 4) ? 4 : 7)
#define PHY_TIMEOUT_MS                (100) /* Default timeout for PHY operations in ms */

#define PHY_THREAD_STACK_SIZE         (1024)
#define PHY_THREAD_PRIORITY           (15)
#define PHY_THREAD_PREMPTION_PRIORITY (15)
#define PHY_THREAD_INTERVAL           (300) /* TODO: Execute once per second (1000) when PHY interrupts work  */

/* ---------------------------------------------------------------------------- */
/* STP Config */
/* ---------------------------------------------------------------------------- */

#define STP_THREAD_STACK_SIZE         (1024)
#define STP_THREAD_PRIORITY           (11)
#define STP_THREAD_PREMPTION_PRIORITY (11)

#define STP_MEM_POOL_SIZE             (2 * 1024 * sizeof(uint8_t))

/* ---------------------------------------------------------------------------- */
/* Commmunications Config */
/* ---------------------------------------------------------------------------- */

#define COMMS_THREAD_STACK_SIZE             (1024 * 32)
#define COMMS_THREAD_PRIORITY               (12)
#define COMMS_THREAD_PREMPTION_PRIORITY     (12)

#define ZENOH_MEM_POOL_SIZE                 (1024 * 32)
#define ZENOH_OPEN_SESSION_INTERVAL         (200) /* ms, time between attempts to open a session */
#define ZENOH_MAX_RETRIES_BEFORE_LONG_PAUSE (5)   /* After this number of failed attempts to open a session pause for Z_TRANSPORT_LEASE to reset any leases on remote devices */

#define ZENOH_MODE                          Z_CONFIG_MODE_CLIENT
#define ZENOH_LOCATOR                       "" /* Empty means it will scout. Otherwise: "udp/192.168.50.2:7447" */

#define ZENOH_PUB_HEARTBEAT_KEYEXPR         NODE_ID   "/status"       /* The topic to send heartbeats to */
#define ZENOH_SUB_HEARTBEAT_KEYEXPR         SERVER_ID "/status"       /* The topic to receive heartbeats from */

#define ZENOH_SUB_STDIN_KEYEXPR             NODE_ID   "/stdin"
#define ZENOH_PUB_STDOUT_KEYEXPR            NODE_ID   "/stdout"
#define ZENOH_PUB_STDERR_KEYEXPR            NODE_ID   "/stderr"

#define ZENOH_PUB_STATS_KEYEXPR             NODE_ID   "/switch-stats" /* The topic to publish switch stats to */

#define HEARTBEAT_INTERVAL                  (500)  /* ms */
#define HEARTBEAT_MISS_TIMEOUT              (2000) /* ms, if the time between heartbeats is larger than this value then assume the producer has disconnected */

/* ---------------------------------------------------------------------------- */
/* Background Thread Config */
/* ---------------------------------------------------------------------------- */

#define BACKGROUND_THREAD_STACK_SIZE         (1024)
#define BACKGROUND_THREAD_PRIORITY           (15)
#define BACKGROUND_THREAD_PREMPTION_PRIORITY (15)

#define BACKGROUND_THREAD_INTERVAL           (1000) /* ms, how often to run */


#ifdef __cplusplus
}
#endif

#endif /* INC_CONFIG_H_ */
