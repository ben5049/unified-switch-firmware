/*
 * sequencer.c
 *
 *  Created on: Sep 3, 2025
 *      Author: bens1
 */

#include "hal.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "sequencer.h"
#include "switch.h"
#include "phy.h"
#include "stp_thread.h"
#include "comms_thread.h"
#include "ptp.h"
#include "podl.h"
#include "utils.h"


TX_THREAD            sequencer_thread_handle;
uint8_t              sequencer_thread_stack[SEQUENCER_THREAD_STACK_SIZE];
TX_EVENT_FLAGS_GROUP sequencer_events_handle;


void sequencer_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status   = TX_SUCCESS;
    nx_status_t nx_status   = NX_SUCCESS;
    uint32_t    event_flags = 0;
    uint32_t    ip_address  = 0;
    uint32_t    subnet_mask = 0;

    /* Startup sequence */

    tx_status = tx_thread_resume(&phy_thread_handle);
    TX_CHECK(tx_status);
    LOG_INFO("PHY Thread started");

    tx_status = tx_thread_resume(&nx_link_thread_handle);
    TX_CHECK(tx_status);
    LOG_INFO("NX Link thread started");

    tx_status = tx_thread_resume(&switch_thread_handle);
    TX_CHECK(tx_status);
    LOG_INFO("Switch thread started");

    podl_enable();

    /* -------------------- Link Up -------------------- */

    /* Wait for the link to be up (from nx_link_thread_entry) */
    LOG_INFO("Waiting for link up");
    tx_status = tx_event_flags_get(&sequencer_events_handle, STATE_MACHINE_NX_LINK_UP, TX_OR_CLEAR, &event_flags, TX_WAIT_FOREVER);
    TX_CHECK(tx_status);
    LOG_INFO("Link is up");

#if FEAT_STP
    tx_status = tx_thread_resume(&stp_thread_handle);
    TX_CHECK(tx_status);
    LOG_INFO("STP thread started");
#endif

    /* Start the PTP thread. No IP address is required since gPTP uses raw ethernet frames */
#if FEAT_PTP
    tx_status = ptp_start();
    TX_CHECK(tx_status);
    LOG_INFO("PTP started");
#endif

    /* -------------------- Network Up -------------------- */

    /* Wait for the network to be initialised and an IP address assigned */
    LOG_INFO("Waiting for network up");
    tx_status = tx_event_flags_get(&sequencer_events_handle, STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED, TX_OR_CLEAR, &event_flags, TX_WAIT_FOREVER);
    TX_CHECK(tx_status);

    /* Print the IP address */
    nx_status = nx_ip_address_get(&nx_ip_instance, (ULONG *) &ip_address, (ULONG *) &subnet_mask);
    NX_CHECK(nx_status);
    LOG_INFO("Network is up, IP Address = %u.%u.%u.%u",
             U32_BYTE_3(ip_address),
             U32_BYTE_2(ip_address),
             U32_BYTE_1(ip_address),
             U32_BYTE_0(ip_address));

    /* Start the threads that require IP networking */
#if FEAT_COMMS
    tx_status = tx_thread_resume(&comms_thread_handle);
    TX_CHECK(tx_status);
    LOG_INFO("Comms thread started");
#endif

    tx_status = tx_thread_terminate(&sequencer_thread_handle);
    TX_CHECK(tx_status);
}
