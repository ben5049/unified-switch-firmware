/*
 * ptp_client.c
 *
 *  Created on: June 4, 2026
 *      Author: bens1
 */

#include "app.h"
#include "ptp.h"


NX_PTP_CLIENT  ptp_client[NUM_PHYS];
static uint8_t nx_internal_ptp_stack[NUM_PHYS][NX_INTERNAL_PTP_THREAD_STACK_SIZE];

TX_MUTEX ptp_client_event_queue_mutex_handle;


/* Note: This function should only be called by the thread that solely consumes this queue. Keep takes priority over remove */
tx_status_t ptp_client_filter_event_queue(port_bitset_t keep_ports, port_bitset_t remove_ports) {

    tx_status_t             status = TX_SUCCESS;
    ptp_client_event_info_t event_info;
    uint32_t                enqueued;
    bool                    keep_event;

    /* Flush all ports */
    if ((keep_ports == PORT_BITS_NONE) && (remove_ports == PORT_BITS_ALL)) {
        status = tx_queue_flush(&ptp_event_queue);
        if (status != TX_SUCCESS) return status;
    }

    /* Loop until all events have been filtered */
    else {

        status = tx_mutex_get(&ptp_client_event_queue_mutex_handle, 100);
        TX_CHECK(status);

        /* Get number of events to filter */
        status = tx_queue_info_get(&ptp_event_queue, NULL, &enqueued, NULL, NULL, NULL, NULL);
        if (status != TX_SUCCESS) return status;

        /* Iterate through event indexes */
        for (uint_fast32_t i = 0; i < enqueued; i++) {

            /* Get an event */
            status = tx_queue_receive(&ptp_event_queue, &event_info, TX_NO_WAIT);

            /* Check the port of the event */
            if (status == TX_SUCCESS) {

                assert((event_info.event == PTP_CLIENT_EVENT_MASTER) ||
                       (event_info.event == PTP_CLIENT_EVENT_SYNC) ||
                       (event_info.event == PTP_CLIENT_EVENT_TIMEOUT));

                keep_event = (keep_ports & (1UL << event_info.port)) ||
                             ((keep_ports == PORT_BITS_NONE) && !(remove_ports & (1UL << event_info.port)));

                /* Re-add the event to the back of the queue */
                if (keep_event) {
                    status = tx_queue_send(&ptp_event_queue, &event_info, TX_NO_WAIT);
                    if (status != TX_SUCCESS) return status;
                }
            }

            /* Empty queue */
            else if (status == TX_QUEUE_EMPTY) {
                status = TX_SUCCESS;
                break;
            }

            /* Unknown error */
            else {
                break;
            }
        }

        status = tx_mutex_put(&ptp_client_event_queue_mutex_handle);
        TX_CHECK(status);
    }

    return status;
}


tx_status_t ptp_client_event_queue_send(ptp_client_event_info_t *event_info) {

    tx_status_t status = TX_SUCCESS;

    status = tx_mutex_get(&ptp_client_event_queue_mutex_handle, 100);
    TX_CHECK(status);
    status = tx_queue_send(&ptp_event_queue, event_info, TX_NO_WAIT);
    TX_CHECK(status);
    status = tx_mutex_put(&ptp_client_event_queue_mutex_handle);
    TX_CHECK(status);
    status = tx_event_flags_set(&ptp_events_group, PTP_EVENT_CLIENT, TX_OR);
    TX_CHECK(status);

    return status;
}


nx_status_t ptp_client_start(port_index_t i, uint8_t *port_identity) {

    nx_status_t status = NX_SUCCESS;

    assert(i < NUM_PHYS);
    assert(port_identity != NULL);

    /* Start the client */
    status = nx_ptp_client_start(
        &ptp_client[i],
        port_identity,
        NX_PTP_CLOCK_PORT_IDENTITY_SIZE,
        PTP_DOMAIN,
        NX_PTP_TRANSPORT_SPECIFIC_802,
        &ptp_event_callback,
        (void *) i);
    if (status != NX_SUCCESS) return status;

    /* Remove the callback it adds, we intercept and route packets manually */
    status = nx_link_packet_receive_callback_remove(
        ptp_client[i].nx_ptp_client_ip_ptr,
        ptp_client[i].nx_ptp_client_interface_index,
        &(ptp_client[i].nx_ptp_client_link_queue));
    if (status != NX_SUCCESS) return status;

    return status;
}


nx_status_t ptp_client_reset(port_index_t i, bool master) {

    nx_status_t status = NX_SUCCESS;

    assert(i < NUM_PHYS);

    /* Delete the PTP client */
    status = nx_ptp_client_delete(&ptp_client[i]);
    if (status != NX_SUCCESS) return status;

    /* Create the PTP client */
    status = nx_ptp_client_create(
        &ptp_client[i],
        &nx_ip_instance,
        PRIMARY_INTERFACE,
        &nx_small_packet_pool,
        NX_INTERNAL_PTP_EVENT_THREAD_PRIORITY,
        (UCHAR *) nx_internal_ptp_stack[i],
        sizeof(nx_internal_ptp_stack[i]),
        &ptp_clock_callback,
        (void *) i);
    if (status != NX_SUCCESS) return status;

    /* Enable the default master configuration if required */
    if (master) {
        status = nx_ptp_client_master_enable(
            &ptp_client[i],
            NX_PTP_CLIENT_ROLE_SLAVE_AND_MASTER,
            NX_PTP_CLIENT_MASTER_PRIORITY,
            PTP_CLIENT_MASTER_SUB_PRIORITY,
            NX_PTP_CLIENT_MASTER_CLOCK_CLASS,
            NX_PTP_CLIENT_MASTER_ACCURACY,
            NX_PTP_CLIENT_MASTER_CLOCK_VARIANCE,
            NX_PTP_CLIENT_MASTER_CLOCK_STEPS_REMOVED,
            NX_PTP_MASTER_TIME_SRC_INTERNAL_OSCILLATOR);
        if (status != NX_SUCCESS) return status;
    }
    return status;
}


/* Note: In this function clients may generate events before other clients have
 *       been started. Since events are serialised and accessed via a queue,
 *       this is not an issue */
nx_status_t ptp_client_restart_all() {

    nx_status_t status = NX_SUCCESS;
    uint8_t     port_identity[NX_PTP_CLOCK_PORT_IDENTITY_SIZE]; /* 64-bit EUI + port */

    /* Reset clients */
    for (port_index_t i = PORT0; i < NUM_PHYS; i++) {
        status = ptp_client_reset(i, true);
        if (status != NX_SUCCESS) return status;
    }

    /* Reset other state */
    ptp_port_connected_to_master = PORT_HOST;
    if (ptp_client_filter_event_queue(PORT_BITS_NONE, PORT_BITS_ALL) != TX_SUCCESS) {
        status = NX_TX_ERROR;
        if (status != NX_SUCCESS) return status;
    }

    /* Set port EUI */
    write_port_identity_eui(port_identity);

    /* Start the PTP clients */
    for (port_index_t i = PORT0; i < NUM_PHYS; i++) {
        write_port_identity_number(port_identity, i);
        status = ptp_client_start(i, port_identity);
        if (status != NX_SUCCESS) return status;
    }

    return status;
}
