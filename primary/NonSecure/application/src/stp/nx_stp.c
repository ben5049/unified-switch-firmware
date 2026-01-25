/*
 * nx_stp.c
 *
 *  Created on: Aug 10, 2025
 *      Author: bens1
 *
 *  Wrapper around netx functions for sending and receiving BPDUs.
 */

#include "nx_api.h"
#include "nx_stm32_eth_driver.h"

#include "nx_app.h"
#include "nx_stp.h"
#include "stp_thread.h"


nx_stp_t      nx_stp;
const uint8_t bpdu_dest_address[BPDU_DST_ADDR_SIZE]      = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00};
const uint8_t bpdu_dest_address_mask[BPDU_DST_ADDR_SIZE] = {0xff, 0xff, 0xff, 0x00, 0x00, 0xff}; /* Bytes 1 and 2 are masked because the SJA1105 switch puts the source port and switch ID tags there */
const uint8_t bpdu_llc[BPDU_LLC_SIZE]                    = {0x42, 0x42, 0x03};


nx_status_t nx_stp_init(NX_IP *ip_ptr, char *name, TX_EVENT_FLAGS_GROUP *events) {

    nx_status_t status = NX_STATUS_SUCCESS;

    nx_stp.ip_ptr               = ip_ptr;
    nx_stp.tx_packet_ptr        = NULL;
    nx_stp.rx_packet_queue_head = NULL;
    nx_stp.rx_packet_queue_tail = NULL;
    nx_stp.events               = events;

    /* Join the multicast group for spanning tree protocols */
    nx_link_multicast_join(
        ip_ptr,
        PRIMARY_INTERFACE,
        (uint32_t) ((bpdu_dest_address[0] << 8) | bpdu_dest_address[1]),
        (uint32_t) ((bpdu_dest_address[2] << 24) | (bpdu_dest_address[3] << 16) | (bpdu_dest_address[4] << 8) | bpdu_dest_address[5]));

    return status;
}


nx_status_t nx_stp_allocate_packet() {

    nx_status_t status     = NX_SUCCESS;
    NX_PACKET **packet_ptr = &nx_stp.tx_packet_ptr;

    /* Check NetX has been initialised */
    if (nx_stp.ip_ptr->nx_ip_initialize_done != NX_TRUE) status = NX_STATUS_NOT_ENABLED;
    if (status != NX_SUCCESS) return status;

    /* Check a packet hasn't already been allocated */
    if (*packet_ptr != NULL) status = NX_STATUS_PTR_ERROR;
    if (status != NX_SUCCESS) return status;

    /* Allocate a packet */
    status = nx_packet_allocate(&nx_packet_pool, packet_ptr, NX_PHYSICAL_HEADER, TX_WAIT_FOREVER);
    if (status != NX_SUCCESS) return status;

    /* Check there is available space in the packet for the header */
    if ((ULONG) ((*packet_ptr)->nx_packet_prepend_ptr - (*packet_ptr)->nx_packet_data_start) < (BPDU_HEADER_SIZE - BPDU_LLC_SIZE)) status = NX_STATUS_PACKET_OFFSET_ERROR;
    if (status != NX_SUCCESS) return status;

    /* Assign the interface (this is done so the send request is passed to the ethernet driver).
     * Note that interface 1 is the loopback interface so use the first actual interface (0).
     */
    nx_stp.tx_packet_ptr->nx_packet_address.nx_packet_interface_ptr = &(nx_stp.ip_ptr->nx_ip_interface[PRIMARY_INTERFACE]);

    return status;
}


nx_status_t nx_stp_send_packet() {

    nx_status_t status     = NX_SUCCESS;
    NX_PACKET  *packet_ptr = nx_stp.tx_packet_ptr;
    uint32_t   *ethernet_frame_ptr;

    /* Check packet is allocated */
    if (nx_stp.tx_packet_ptr == NULL) status = NX_STATUS_INVALID_PACKET;
    if (status != NX_STATUS_SUCCESS) return status;

    /* Check packet length */
    if (nx_stp.tx_packet_ptr->nx_packet_length == 0) status = NX_STATUS_INVALID_PACKET;
    if (status != NX_STATUS_SUCCESS) return status;

    /* Setup the ethernet frame pointer. Backup another 2 bytes to get 32-bit word alignment */
    ethernet_frame_ptr = (uint32_t *) (packet_ptr->nx_packet_prepend_ptr - 2);

    /* Change the endianess if required. TODO: Does the data need to have its endianess changed? */
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_frame_ptr));
    ethernet_frame_ptr++;
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_frame_ptr));
    ethernet_frame_ptr++;
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_frame_ptr));
    ethernet_frame_ptr++;
    NX_CHANGE_ULONG_ENDIAN(*(ethernet_frame_ptr));
    ethernet_frame_ptr++;

    /* Send the packet */
    status = nx_link_raw_packet_send(nx_stp.ip_ptr, PRIMARY_INTERFACE, packet_ptr);

    /* Clear packet pointer to prevent retransmission */
    nx_stp.tx_packet_ptr = NULL;

    /* Add debug information */
    NX_PACKET_DEBUG(__FILE__, __LINE__, packet_ptr);

    return status;
}


/* Modelled after _nx_rarp_packet_deferred_receive() */
nx_status_t nx_stp_packet_deferred_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr) {

    nx_status_t status = NX_STATUS_SUCCESS;

    TX_INTERRUPT_SAVE_AREA

    /* Disable interrupts */
    TX_DISABLE

    /* Check to see if the STP deferred processing queue is empty */
    if (nx_stp.rx_packet_queue_head) {

        /* Not empty, place the packet at the end of the STP deferred queue */
        nx_stp.rx_packet_queue_tail->nx_packet_queue_next = packet_ptr;
        packet_ptr->nx_packet_queue_next                  = NX_NULL;
        nx_stp.rx_packet_queue_tail                       = packet_ptr;

        /* Restore interrupts */
        TX_RESTORE

    } else {

        /* Empty STP deferred receive processing queue. Setup the head pointers and
           set the event flags to ensure the STP thread looks at the STP deferred
           processing queue */
        nx_stp.rx_packet_queue_head      = packet_ptr;
        nx_stp.rx_packet_queue_tail      = packet_ptr;
        packet_ptr->nx_packet_queue_next = NX_NULL;

        /* Restore interrupts */
        TX_RESTORE

        /* Wake up STP thread to process the STP deferred receive */
        status = tx_event_flags_set(nx_stp.events, STP_BPDU_REC_EVENT, TX_OR);
    }

    return status;
}
