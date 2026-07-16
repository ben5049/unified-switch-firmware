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
#include "tx_user.h"
#include "nx_api.h"
#include "zenoh_generic_platform.h"


/* ---------------------------------------------------------------------------- */
/* Feature Flags */
/* ---------------------------------------------------------------------------- */

#define FEAT_DHCP_RESTORE         (1)
#define FEAT_PHY_SELF_TEST        (1) // TODO: Use
#define FEAT_PTP                  (1)
#define FEAT_PTP_SWITCH_SYNC      (1) /* Actively synchronise the switches' clocks to each other. Turning it off enables a PPS signal on the PTP_CLK pin instead */
#define FEAT_PTP_ALLOW_META_DELAY (1) /* Allow META frames to have up to one packet between them and the PTP packet that generated them */
#define FEAT_PTP_PPS_SOFT         (1) /* Trigger a software callback once per second */
#define FEAT_STP                  (0) /* More important TODO: replace with custom implementation, MSTP is too heavy. TODO: fix this thread. Each time it calls for a flush the 50MHz REF_CLK is reset which is probably very bad */
#define FEAT_COMMS                (0) /* TODO: re-enable */

/* ---------------------------------------------------------------------------- */
/* Common Config */
/* ---------------------------------------------------------------------------- */

#define DEFAULT_SECURE_STACK_SIZE (1024) /* Any thread that calls secure functions should allocate this ammount of secure stack */

/* ---------------------------------------------------------------------------- */
/* Logging Config */
/* ---------------------------------------------------------------------------- */

#define NUM_LOGGERS         (8)
#define LOG_TIMEOUT         (100)  /* ms */
#define UART_LOGGING_ENABLE (true) /* Note: must alse be enabled in secure firmware config */

/* ---------------------------------------------------------------------------- */
/* Trace Config */
/* ---------------------------------------------------------------------------- */

#define TRACE_ENABLE           (false)
#define TRACE_REGISTRY_ENTRIES (30)

/* ---------------------------------------------------------------------------- */
/* State Machine Config */
/* ---------------------------------------------------------------------------- */

#define SEQUENCER_THREAD_STACK_SIZE   (1024)
#define STATE_MACHINE_THREAD_PRIORITY (9)

/* ---------------------------------------------------------------------------- */
/* Networking Common Config */
/* ---------------------------------------------------------------------------- */

// TODO: pass in from cmake
#define MAC_ADDR_OCTET1                  (0x02)
#define MAC_ADDR_OCTET2                  (0x00)
#define MAC_ADDR_OCTET3                  (0x00)
#define MAC_ADDR_OCTET4                  (HW_VERSION)
#define MAC_ADDR_OCTET5                  (0x6a)
#define MAC_ADDR_OCTET6                  (0x48)

#define SMALL_PACKET_SIZE                (128)
#define NUM_SMALL_PACKETS                (80)

#define BIG_PACKET_SIZE                  (1536) /* Ethernet payload size field (0x600) */
#define NUM_BIG_PACKETS                  (24)   /* Only big packets can be used for receiving */

#define NX_APP_DEFAULT_TIMEOUT           (1000) /* Generic timeout for nx events (e.g. TCP send) in ms */

#define NX_DEFAULT_IP_ADDRESS            (0)    /* TODO: Set this */
#define NX_DEFAULT_NET_MASK              (0)    /* TODO: Set this */
#define NX_INTERNAL_IP_THREAD_STACK_SIZE (2 * 1024)
#define NX_INTERNAL_IP_THREAD_PRIORITY   (NX_APP_THREAD_PRIORITY)

#define DEFAULT_ARP_CACHE_SIZE           (1024)

#define NX_APP_THREAD_STACK_SIZE         (2 * 1024)
#define NX_APP_THREAD_PRIORITY           (10)

#define NUM_VLANS                        (8) /* Currently only used for STP (unused due to RSTP not MSTP) */

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

/* ---------------------------------------------------------------------------- */
/* Link Config */
/* ---------------------------------------------------------------------------- */

#define NX_LINK_THREAD_STACK_SIZE            (2 * 1024)
#define NX_LINK_THREAD_PRIORITY              (9)

#define NX_APP_LINK_CHECK_WHEN_DOWN_INTERVAL (200)  /* Interval between link checks in ms when the link is down */
#define NX_APP_LINK_CHECK_WHEN_UP_INTERVAL   (1000) /* Interval between link checks in ms when the link is up */

/* ---------------------------------------------------------------------------- */
/* PTP Config */
/* ---------------------------------------------------------------------------- */

#define NX_INTERNAL_PTP_THREAD_STACK_SIZE     (1536)
#define NX_INTERNAL_PTP_EVENT_THREAD_PRIORITY (8)

#define PTP_EVENT_THREAD_STACK_SIZE           (1024 * 2)
#define PTP_EVENT_THREAD_PRIORITY             (7)
#define PTP_EVENT_QUEUE_SIZE                  (10)
#define PTP_PRINT_TIME_INTERVAL               (10000) /* Time interval between printing the PTP time in ms. Must be >= 100ms. Set to 0 to disable printing */
#define PTP_SYNC_TIMEOUT                      (10000) /* A SYNC event should generated within this amount of time after connecting to a master */

#define PTP_PPS_SOFT_PULSE_DURATION           (100)

#define PTP_TX_THREAD_STACK_SIZE              (1024 * 2)
#define PTP_TX_THREAD_PRIORITY                (4)
#define PTP_TX_QUEUE_SIZE                     MIN(NUM_PHYS * 8, NUM_SMALL_PACKETS / 2) /* Buffer up to 8 transmitted PTP packets per port. Don't use more than half the available packets */

#define PTP_RX_THREAD_STACK_SIZE              (1024 * 2)
#define PTP_RX_THREAD_PRIORITY                (3)
#define PTP_RX_QUEUE_SIZE                     MIN(NUM_PHYS * 3, NUM_BIG_PACKETS / 2) /* Buffer up to 3 received PTP packets per port. Don't use more than half the available packets */
#define PTP_RX_TIMEOUT                        (100)                                  /* How long to wait for a meta frame */

#define PTP_CLOCK_THREAD_STACK_SIZE           (1024 * 2)
#define PTP_CLOCK_THREAD_PRIORITY             (6)
#define PTP_CLOCK_QUEUE_SIZE                  (3) /* Number of adjustments to buffer, if this queue fills up then it is flushed and the latest adjustment is resent so small size is fine */
#define PTP_CLOCK_TIMEOUT                     (200)

#define PTP_CLOCK_PRINT_OFFSET                (0)
#define PTP_CLOCK_FINE_ADJUST_THRESHOLD       (10)                               /* ms, the PI controller will be used for errors less than this */
#define PTP_CLOCK_CONTROLLER_KP               (2.0f)                             /* Proportional gain */
#define PTP_CLOCK_CONTROLLER_KI               (0.25f)                            /* Integral gain */
#define PTP_CLOCK_CONTROLLER_INTEGRAL_MAX     (1000000LL)                        /* Integral saturation */
#define PTP_CLOCK_CONTROLLER_MIN_RATE         (SJA1105_PTP_CLK_RATE_MUCH_SLOWER) /* Corresponds to -1% */
#define PTP_CLOCK_CONTROLLER_MAX_RATE         (SJA1105_PTP_CLK_RATE_MUCH_FASTER) /* Corresponds to +1% */

#if FEAT_PTP_SWITCH_SYNC
#define PTP_SWITCH_SYNC_INTERVAL                (100)
#define PTP_SWITCH_SYNC_PRINT_OFFSET            (0)
#define PTP_SWITCH_SYNC_CONTROLLER_KP           (1.0f)                             /* Proportional gain */
#define PTP_SWITCH_SYNC_CONTROLLER_KI           (0.05f)                            /* Integral gain */
#define PTP_SWITCH_SYNC_CONTROLLER_INTEGRAL_MAX (1000000LL)                        /* Integral saturation */
#define PTP_SWITCH_SYNC_CONTROLLER_MIN_RATE     (SJA1105_PTP_CLK_RATE_MUCH_SLOWER) /* Corresponds to -1% */
#define PTP_SWITCH_SYNC_CONTROLLER_MAX_RATE     (SJA1105_PTP_CLK_RATE_MUCH_FASTER) /* Corresponds to +1% */
#endif

#define PTP_MAC_SYNC_THREAD_STACK_SIZE       (1024 * 2)
#define PTP_MAC_SYNC_THREAD_PRIORITY         (5)
#define PTP_MAC_SYNC_INTERVAL                (TX_TIMER_TICKS_PER_SECOND / 16) /* PTP Sync events happen at 8Hz and MAC syncs are an inner loop inside those, therefore must have at least double the frequency */
#define PTP_MAC_SYNC_QUEUE_SIZE              (4)                              /* 4 timestamps for offset calculation: MAC TX/RX and switch TX/RX */

#define PTP_MAC_SYNC_PRINT_OFFSET            (0)
#define PTP_MAC_SYNC_FINE_ADJUST_THRESHOLD   (10)         /* ms, the PI controller will be used for errors less than this */
#define PTP_MAC_SYNC_CONTROLLER_KP           (20.0f)      /* Proportional gain */
#define PTP_MAC_SYNC_CONTROLLER_KI           (20.0f)      /* Integral gain */
#define PTP_MAC_SYNC_CONTROLLER_INTEGRAL_MAX (1000000LL)  /* Integral saturation */
#define PTP_MAC_SYNC_CONTROLLER_MIN_RATE     (-10000000L) /* Corresponds to -1% */
#define PTP_MAC_SYNC_CONTROLLER_MAX_RATE     (10000000L)  /* Corresponds to +1% */

#define PTP_CLIENT_MASTER_SUB_PRIORITY       (248)        /* The subpriority of this device for BMCA. Default for an end instance is 248 */
#define PTP_DOMAIN                           (0)
#define PTP_VLAN                             (0)

/* Management route variables */
#define PTP_TX_TIMEOUT (500) /* The maximum number of ms to wait to send a packet */
#define PTP_TX_TSREG   (0)

/* ---------------------------------------------------------------------------- */
/* Switch Config */
/* ---------------------------------------------------------------------------- */

#define SWITCH_TIMEOUT_MS                 (100)  /* Default timeout for switch operations in ms */
#define SWITCH_MANAGMENT_ROUTE_TIMEOUT_MS (1000) /* The time after allocating a management route when that route can be freed if not used */

#define SWITCH_THREAD_STACK_SIZE          (4 * 1024)
#define SWITCH_THREAD_PRIORITY            (15)

#define SWITCH_MAINTENANCE_INTERVAL       (500)                     /* Time between performing switch maintenance operations in ms */
#define SWITCH_PUBLISH_STATS_INTERVAL     (1000)                    /* Time between publishing switch statistic in ms */
#define SWITCH_GET_EXTENDED_STATS         (DEBUG ? true : false)    /* Get the extented statistics too (uses lots of stack) */

#define SWITCH_MEM_POOL_SIZE              (1024 * sizeof(uint32_t)) /* 1024 Words should be enough for most variable length tables. TODO: Check */

#define SWITCH_ERR_THRESHOLD              (10)                      /* Once this number of errors has occured then reset the switch to prevent memory leaks */

/* ---------------------------------------------------------------------------- */
/* PHY Config */
/* ---------------------------------------------------------------------------- */

#define PHY_TIMEOUT_MS                        (100) /* Default timeout for PHY operations in ms */

#define PHY_THREAD_STACK_SIZE                 (2 * 1024)
#define PHY_THREAD_PRIORITY                   (9)   /* Higher priority than IP thread */
#define PHY_THREAD_INTERVAL                   (100) /* Execute frequently, work done is dependent on PHY state machines so higher frequency doesn't mean more computation */

#define PHY_TEMPERATURE_READ_INTERVAL         (1000)

#define PHY_WAITING_FOR_LINK_INTERVAL         (200)
#define PHY_WAITING_FOR_LINK_INTERVAL_AUTONEG (2500) /* Auto-negotiation takes a reallly long time */
#define PHY_WAITING_FOR_LINK_ATTEMPTS         (2)
#define PHY_WAITING_FOR_LINK_TIME             (PHY_WAITING_FOR_LINK_ATTEMPTS * PHY_WAITING_FOR_LINK_INTERVAL)

#define PHY_RECONNECT_INTERVAL                (100)
#define PHY_RECONNECT_ATTEMPTS                (20)

#define PHY_SLEEP_INTERVAL                    (PHY_WAITING_FOR_LINK_TIME * 4) /* 80% Of the time spent asleep */

#define PHY_SELF_TEST_ON_STARTUP              (true)                          /* Note that the PHY will make one attempt at linkup first */
#define PHY_SELF_TEST_INTERVAL                (1000 * 60 * 10)                /* Every 10 minutes */

#define PHY_POLL_STAGGERING                   (true)                          /* Stagger polling loops so all the PHYs don't wake up at the same time to check for links */

#if HW_VERSION == 4
#define PHY_T_RESET_WIDTH (MAX(PHY_88Q211X_T_RESET, PHY_LAN867X_T_RSTIA))
#define PHY_T_RESET_MDIO  (MAX(PHY_88Q211X_T_RESET_MDIO, PHY_LAN867X_T_CSH))
#elif HW_VERSION == 5
#define PHY_T_RESET_WIDTH (MAX3(PHY_88Q211X_T_RESET, PHY_LAN867X_T_RSTIA, PHY_DP83867_RST_T4))
#define PHY_T_RESET_MDIO  (MAX3(PHY_88Q211X_T_RESET_MDIO, PHY_LAN867X_T_CSH, PHY_DP83867_RST_T1))
#endif

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
#define ZENOH_MAX_RETRIES_BEFORE_LONG_PAUSE (50)  /* After this number of failed attempts to open a session pause for Z_TRANSPORT_LEASE to reset any leases on remote devices */

#define ZENOH_MODE                          Z_CONFIG_MODE_CLIENT
#define ZENOH_LOCATOR                       ""                  /* Empty means it will scout. Otherwise: "udp/192.168.50.2:7447" */

#define ZENOH_PUB_HEARTBEAT_KEYEXPR         NODE_ID "/status"   /* The topic to send heartbeats to */
#define ZENOH_SUB_HEARTBEAT_KEYEXPR         SERVER_ID "/status" /* The topic to receive heartbeats from */

#define ZENOH_SUB_STDIN_KEYEXPR             NODE_ID "/stdin"
#define ZENOH_PUB_STDOUT_KEYEXPR            NODE_ID "/stdout"
#define ZENOH_PUB_STDERR_KEYEXPR            NODE_ID "/stderr"

#define ZENOH_PUB_STATS_KEYEXPR             NODE_ID "/switch/stats" /* The topic to publish switch stats to */

#define HEARTBEAT_INTERVAL                  (500)                   /* ms */
#define HEARTBEAT_MISS_TIMEOUT              (2000)                  /* ms, if the time between heartbeats is larger than this value then assume the producer has disconnected */

/* ---------------------------------------------------------------------------- */
/* Background Thread Config */
/* ---------------------------------------------------------------------------- */

#define BACKGROUND_THREAD_STACK_SIZE (1024 * 1)
#define BACKGROUND_THREAD_PRIORITY   (15)
#define BACKGROUND_THREAD_INTERVAL   (1000) /* ms, how often to run */

/* ---------------------------------------------------------------------------- */
/* Validation */
/* ---------------------------------------------------------------------------- */

/* Can't use parentheses due to macro shenanigans */

#define VALIDATION_ENABLE          1 /* Set to 0 to disable all validation (coverage & fault injection) */
#define VALIDATION_FAULT_INJECTION 0 /* Set to 1 to enable random fault injection */
#define VALIDATION_TERMINATE       1 /* Set to 1 to stop on VAL_TERMINATE() calls so the state can be inspected */

#define VALIDATION_SEED            0 /* Seed for the psuedo random number generator. 0 Means true random seed */

/* Unit enables */
#define VALIDATION_PTP_TX       1
#define VALIDATION_PTP_RX       1
#define VALIDATION_PTP_EVENT    1
#define VALIDATION_PTP_CLOCK    1
#define VALIDATION_SWITCH_UTILS 1
#define VALIDATION_PHY          1


#ifdef __cplusplus
}
#endif

#endif /* INC_CONFIG_H_ */
