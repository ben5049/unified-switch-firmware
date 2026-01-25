/*
 * state_machine.c
 *
 *  Created on: Sep 3, 2025
 *      Author: bens1
 */

#include "main.h"
#include "eth.h"

#include "state_machine.h"
#include "tx_app.h"
#include "nx_app.h"
#include "nx_link_thread.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "phy_thread.h"
#include "stp_thread.h"
#include "comms_thread.h"
#include "ptp_thread.h"
#include "config.h"
#include "utils.h"


TX_THREAD            state_machine_thread_handle;
uint8_t              state_machine_thread_stack[STATE_MACHINE_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP state_machine_events_handle;


void state_machine_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status            = TX_SUCCESS;
    nx_status_t nx_status            = NX_SUCCESS;
    uint32_t    event_flags          = 0;
    uint32_t    event_flags_previous = 0;
    uint32_t    ip_address           = 0;
    uint32_t    subnet_mask          = 0;

    /* Startup sequence */

    tx_status = tx_thread_resume(&phy_thread_handle);
    if (tx_status != TX_SUCCESS) Error_Handler();
    ns_log_write("PHY Thread started\n");

    tx_status = tx_thread_resume(&nx_link_thread_handle);
    if (tx_status != TX_SUCCESS) Error_Handler();
    ns_log_write("NX Link thread started\n");

    tx_status = tx_thread_resume(&switch_thread_handle);
    if (tx_status != TX_SUCCESS) Error_Handler();
    ns_log_write("Switch thread started\n");

    /* -------------------- Link Up -------------------- */

    /* Wait for the link to be up (from nx_link_thread_entry) */
    ns_log_write("Waiting for link up\n");
    tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_NX_LINK_UP | STATE_MACHINE_UPDATE, TX_AND, &event_flags, TX_WAIT_FOREVER);
    if (tx_status != TX_SUCCESS) Error_Handler();
    tx_event_flags_set(&state_machine_events_handle, ~STATE_MACHINE_UPDATE, TX_AND);
    if (tx_status != TX_SUCCESS) Error_Handler();
    ns_log_write("Link is up\n");

#if ENABLE_STP_THREAD == true
    tx_status = tx_thread_resume(&stp_thread_handle);
    if (tx_status != TX_SUCCESS) Error_Handler();
    ns_log_write("STP thread started\n");
#endif

    /* -------------------- Network Up -------------------- */

    /* Wait for the network to be initialised and an IP address assigned */
    ns_log_write("Waiting for network up\n");
    tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED | STATE_MACHINE_UPDATE, TX_AND, &event_flags, TX_WAIT_FOREVER);
    if (tx_status != TX_SUCCESS) Error_Handler();
    tx_event_flags_set(&state_machine_events_handle, ~STATE_MACHINE_UPDATE, TX_AND);
    if (tx_status != TX_SUCCESS) Error_Handler();

    /* Print the IP address */
    nx_status = nx_ip_address_get(&nx_ip_instance, &ip_address, &subnet_mask);
    if (nx_status != NX_SUCCESS) Error_Handler();
    NX_CHANGE_ULONG_ENDIAN(ip_address);
    uint8_t *bytes = (uint8_t *) &ip_address;
    ns_log_write("Network is up, IP Address = %u.%u.%u.%u\n", bytes[0], bytes[1], bytes[2], bytes[3]);

    /* Start the threads that require networking */
    tx_status = tx_thread_resume(&comms_thread_handle);
    if (tx_status != TX_SUCCESS) Error_Handler();
    ns_log_write("Comms thread started\n");

    // tx_status = tx_thread_resume(&ptp_thread_handle);
    // if (tx_status != TX_SUCCESS) Error_Handler();

    while (1) {

        /* Wait for an update */
        tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_UPDATE, TX_OR_CLEAR, &event_flags, TX_WAIT_FOREVER);
        if (tx_status != TX_SUCCESS) Error_Handler();

        /* Save the previous state */
        event_flags_previous = event_flags;

        UNUSED(event_flags_previous); /* TODO: Remove when used */
    }
}
