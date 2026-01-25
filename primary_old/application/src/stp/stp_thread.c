/*
 * stp_thread.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "hal.h"
#include "main.h"
#include "nx_api.h"
#include "nx_packet.h"
//#include "stp.h"

#include "stp_thread.h"
#include "stp_callbacks.h"
#include "nx_stp.h"
#include "nx_app.h"
#include "tx_app.h"
#include "config.h"
#include "utils.h"
#include "switch_thread.h"
#include "phy_thread.h"
#include "phy_common.h"


uint8_t              stp_thread_stack[STP_THREAD_STACK_SIZE];
TX_THREAD            stp_thread_handle;
TX_EVENT_FLAGS_GROUP stp_events_handle;


volatile uint32_t stp_error_counter = 0;

//STP_BRIDGE* bridge;


//static void poll_port_status_and_call_library(size_t pi, unsigned int now) {
//
//    /* TODO: Implement this properly */
//
//     See Table 59 on page 213 in 88E6352_Functional_Specification-Rev0-08.pdf.
//     uint16_t reg = read_phy_register(pi, 17);
//     if (reg & (1u << 10)) {
//         // link up
//         if (reg & (1u << 11)) {
//             // speed and duplex resolved (or auto-negotiation disabled)
//             if (!STP_GetPortEnabled(bridge, pi)) {
//                 static constexpr uint32_t speeds[] = {10, 100, 1000, (uint32_t) -1};
//                 uint32_t                  speed    = speeds[(reg >> 14) & 3];
//                 bool                      duplex   = reg & (1u << 13);
//                 printf("Port %d Link Up   (", pi);
//                 print_binary(reg);
//                 printf(") Speed %d, %s-duplex.\r\n", speed, duplex ? "Full" : "Half");
//                 STP_OnPortEnabled(bridge, pi, speed, true, now);
//             }
//         }
//     } else {
//         // link down
//         if (STP_GetPortEnabled(bridge, pi)) {
//             printf("Port %d Link Down (", pi);
//             print_binary(reg);
//             printf(").\r\n", pi);
//             STP_OnPortDisabled(bridge, pi, now);
//         }
//     }
//}


/* Check the received BPDU and ignore it if it's malformed */
//static void validate_and_process_bpdu(const uint8_t* data, uint16_t size) {
//
//    /* Don't process the BPDU if the bridge isn't started */
//    if (!STP_IsBridgeStarted(bridge)) {
//        Error_Handler(); /* TODO: This isn't a great idea, should just ignore malformed BPDUs */
//    }
//
//    /* Check the size of the BPDU */
//    if (size < BPDU_HEADER_SIZE) {
//        Error_Handler(); /* TODO: This isn't a great idea, should just ignore malformed BPDUs */
//    }
//
//    uint32_t now = tx_time_get_ms();
//    /*
//            printf ("%u.%03u: RX: %02x:%02x:%02x:%02x:%02x:%02x  %02x:%02x:%02x:%02x:%02x:%02x  %02x%02x\r\n",
//                    now / 1000, now % 1000,
//                    data[0], data[1], data[2], data[3], data[4], data[5],
//                    data[6], data[7], data[8], data[9], data[10], data[11],
//                    data[12], data[13]);
//    */
//
//    uint16_t etherTypeOrSize = (((uint16_t) data[12]) << 8) | data[13];
//
//    /* 3 is the size of the LLC field */
//    if ((etherTypeOrSize < 3) || (etherTypeOrSize > 1536)) {
//        Error_Handler(); /* TODO: This isn't a great idea, should just ignore malformed BPDUs */
//    }
//
//    const uint8_t* bpdu       = data + BPDU_HEADER_SIZE;
//    uint16_t       bpdu_size  = etherTypeOrSize - 3;
//    uint8_t        port_index = data[3]; /* The switch inserts the source port into byte 2 of the destination address */
//
//    /* Check the port is valid */
//    if ((port_index >= SJA1105_NUM_PORTS) || (port_index == PORT_HOST)) {
//        Error_Handler();
//    }
//
//    if (!STP_GetPortEnabled(bridge, port_index)) poll_port_status_and_call_library(port_index, now);
//
//    STP_OnBpduReceived(bridge, port_index, bpdu, bpdu_size, now);
//}


void stp_thread_entry(uint32_t initial_input) {
//
//    uint32_t    event_flags;
//    NX_PACKET*  received_packet;
//    tx_status_t tx_status = TX_SUCCESS;
//    nx_status_t nx_status = NX_SUCCESS;
//
//    static const uint8_t mac_address[6] = {
//        MAC_ADDR_OCTET1,
//        MAC_ADDR_OCTET2,
//        MAC_ADDR_OCTET3,
//        MAC_ADDR_OCTET4,
//        MAC_ADDR_OCTET5,
//        MAC_ADDR_OCTET6};
//
//    /* Create the NetX STP instance used to send BPDUs */
//    nx_status = nx_stp_init(&nx_ip_instance, "nx_stp_instance", &stp_events_handle);
//    if (nx_status != NX_SUCCESS) Error_Handler();
//
//    /* Initialise the STP ThreadX byte pool */
//    tx_status = stp_byte_pool_init();
//    if (tx_status != TX_SUCCESS) Error_Handler();
//
//    /* Create and start the bridge */
//    bridge = STP_CreateBridge(5, 0, NUM_VLANS, &stp_callbacks, mac_address, 100);
//    STP_StartBridge(bridge, tx_time_get_ms());
//
//    /* Setup timing control variables (done in ticks) */
//    uint32_t current_time   = tx_time_get();
//    uint32_t next_wake_time = current_time + TX_TIMER_TICKS_PER_SECOND;
//
//    /* The host port is always enabled by default */
//    STP_OnPortEnabled(bridge, PORT_HOST, 100, true, tx_time_get_ms());
//
//    // TODO: poll ports to check for link up before STP started
//
//    while (1) {
//
//        /* Sleep until the next wake time while also monitoring for received BPDUs */
//        current_time = tx_time_get();
//        if (current_time < next_wake_time) {
//
//            /* Wait for a BPDU */
//            tx_status = tx_event_flags_get(&stp_events_handle, STP_ALL_EVENTS, TX_OR_CLEAR, &event_flags, next_wake_time - current_time);
//            if ((tx_status != TX_SUCCESS) && (tx_status != TX_NO_EVENTS)) Error_Handler();
//
//            /* Process events */
//            if (event_flags && (tx_status != TX_NO_EVENTS)) {
//
//                /* If at least one BPDU was received then process it */
//                if (event_flags & STP_BPDU_REC_EVENT) {
//
//                    TX_INTERRUPT_SAVE_AREA
//
//                    /* Iterate through all BPDUs in the queue */
//                    while (nx_stp.rx_packet_queue_head != NULL) {
//
//                        /* Disable interrupts */
//                        TX_DISABLE
//
//                        /* Pickup the first packet and move the head to the next packet */
//                        received_packet             = nx_stp.rx_packet_queue_head;
//                        nx_stp.rx_packet_queue_head = received_packet->nx_packet_queue_next;
//                        if (nx_stp.rx_packet_queue_head == NX_NULL) nx_stp.rx_packet_queue_tail = NX_NULL;
//
//                        /* Restore interrupts */
//                        TX_RESTORE
//
//                        /* Process and release the BPDU */
//                        validate_and_process_bpdu(received_packet->nx_packet_prepend_ptr, received_packet->nx_packet_length); // TODO: Check return codes
//                        nx_status = _nx_packet_release(received_packet);
//                        if (nx_status != NX_SUCCESS) Error_Handler();
//                    }
//                }
//
//                /* Process link up/down events */
//                if (event_flags & STP_PORT0_LINK_STATE_CHANGE_EVENT) {
//                    if (hphy0.linkup && !STP_GetPortEnabled(bridge, PORT_88Q2112_PHY0)) {
//                        STP_OnPortEnabled(bridge, PORT_88Q2112_PHY0, PHY_GET_SPEED_MBPS(hphy0), true, tx_time_get_ms());
//                    } else {
//                        STP_OnPortDisabled(bridge, PORT_88Q2112_PHY0, tx_time_get_ms());
//                    }
//                }
//                if (event_flags & STP_PORT1_LINK_STATE_CHANGE_EVENT) {
//                    if (hphy1.linkup && !STP_GetPortEnabled(bridge, PORT_88Q2112_PHY1)) {
//                        STP_OnPortEnabled(bridge, PORT_88Q2112_PHY1, PHY_GET_SPEED_MBPS(hphy1), true, tx_time_get_ms());
//                    } else {
//                        STP_OnPortDisabled(bridge, PORT_88Q2112_PHY1, tx_time_get_ms());
//                    }
//                }
//                if (event_flags & STP_PORT2_LINK_STATE_CHANGE_EVENT) {
//                    if (hphy2.linkup && !STP_GetPortEnabled(bridge, PORT_88Q2112_PHY2)) {
//                        STP_OnPortEnabled(bridge, PORT_88Q2112_PHY2, PHY_GET_SPEED_MBPS(hphy2), true, tx_time_get_ms());
//                    } else {
//                        STP_OnPortDisabled(bridge, PORT_88Q2112_PHY2, tx_time_get_ms());
//                    }
//                }
//                if (event_flags & STP_PORT3_LINK_STATE_CHANGE_EVENT) {
//                    if (hphy3.linkup && !STP_GetPortEnabled(bridge, PORT_LAN8671_PHY)) {
//                        STP_OnPortEnabled(bridge, PORT_LAN8671_PHY, 10, false, tx_time_get_ms());
//                    } else {
//                        STP_OnPortDisabled(bridge, PORT_LAN8671_PHY, tx_time_get_ms());
//                    }
//                }
//
//                /* Go to the start of the loop and go back to sleep if necessary */
//                continue;
//            }
//
//            /* Otherwise the sleep time is over and regular STP processing must be done */
//        }
//
//        /* Schedule next wake time in one second */
//        next_wake_time += TX_TIMER_TICKS_PER_SECOND;
//
//        // /* TODO: Fix */
//        // if (hphy0.linkup && !STP_GetPortEnabled(bridge, PORT_88Q2112_PHY0)) {
//        //     STP_OnPortEnabled(bridge, PORT_88Q2112_PHY0, PHY_GET_SPEED_MBPS(hphy0), true, tx_time_get_ms());
//        // }
//
//        /* Do any necessary STP work */
//        STP_OnOneSecondTick(bridge, tx_time_get_ms());
//    }
	tx_thread_relinquish();
}
