/*
 * ptp_rx_thread.c
 *
 *  Created on: May 20, 2026
 *      Author: bens1
 */

#include "assert.h"

#include "nx_stm32_eth_driver.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "utils.h"
#include "ptp_thread.h"
#include "ptp_init.h"
#include "switch_utils.h"


TX_THREAD ptp_rx_thread_handle;
uint8_t   ptp_rx_thread_stack[PTP_RX_THREAD_STACK_SIZE];

TX_QUEUE ptp_rx_packet_queue_handle;
uint32_t ptp_rx_packet_queue_stack[PTP_RX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
TX_QUEUE ptp_rx_meta_queue_handle;
uint32_t ptp_rx_meta_queue_stack[PTP_RX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];


void ptp_rx_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status = TX_SUCCESS;
    nx_status_t nx_status = NX_SUCCESS;

    ptp_event_info_t event_info;

    NX_PACKET  *ptp_packet;
    NX_PTP_TIME timestamp;
    phy_index_t port;

    NX_LINK_RECEIVE_QUEUE *nx_receive_queue_ptr;
    UINT                   header_size;

#if DEBUG
    uint32_t enqueued;
    uint32_t packet_queue_high_water_mark = 0;
#endif

    while (1) {

        tx_status = tx_queue_receive(&ptp_rx_packet_queue_handle, &event_info, TX_WAIT_FOREVER);
#if DEBUG
        tx_queue_info_get(&ptp_rx_packet_queue_handle, NULL, &enqueued, NULL, NULL, NULL, NULL);
        if ((enqueued + 1) > packet_queue_high_water_mark) {
            packet_queue_high_water_mark = (enqueued + 1);
            LOG_INFO("PTP RX Queue high water mark = %lu/%d", packet_queue_high_water_mark, PTP_RX_QUEUE_SIZE);
        }
#endif
        if (tx_status == NX_SUCCESS) {

            switch (event_info.event) {

                case PTP_RX_EVENT_RECEIVE_PACKET: {

                    ptp_packet = event_info.data;

                    /* Get the follow up META frame */
                    tx_status = tx_queue_receive(&ptp_rx_meta_queue_handle, &event_info, MS_TO_TICKS(PTP_RX_TIMEOUT));

                    /* META frame lost, drop the PTP packet since it cannot be timestamped.
                     * Also flush the queues to prevent alignment issues. */
                    if (tx_status != TX_SUCCESS) {
                        nx_packet_release(ptp_packet);

                        /* Flush both queues */
                        ptp_flush_packet_queue(&ptp_rx_packet_queue_handle);
                        ptp_flush_packet_queue(&ptp_rx_meta_queue_handle);

                        ptp_event_counters.rx_no_meta++;
                        LOG_ERROR("PTP RX No META frame found");
                        break;
                    }

                    /* Parse META frame for timestamp and port then free it.
                     * Store the resulting timestamp in the packet */
                    if (switch_parse_and_free_meta_frame(event_info.data, &port, &timestamp) != SJA1105_OK) error_handler();
                    assert(port < NUM_PHYS);
                    ptp_packet_insert_timestamp(ptp_packet, &timestamp);

                    /* Iterate through the receive callbacks to find the one for this port */
                    nx_receive_queue_ptr = nx_ip_instance.nx_ip_interface[PRIMARY_INTERFACE].nx_interface_link_receive_queue_head;
                    while (nx_receive_queue_ptr) {

                        /* PTP Client found */
                        if ((NX_PTP_CLIENT *) nx_receive_queue_ptr->context == &ptp_client[port]) {

                            /* Parse information from the packet header */
                            nx_status = nx_link_ethernet_header_parse(ptp_packet, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &header_size);
                            if (nx_status != NX_SUCCESS) error_handler();

                            /* Call the packet receive callback
                             *
                             * Note: _nx_ptp_client_ethernet_receive_notify doesn't use the following:
                             *   - ip_ptr
                             *   - interface_index
                             *   - physical_address_msw
                             *   - physical_address_lsw
                             *   - packet_type
                             *   - time_ptr
                             */
                            nx_status = nx_receive_queue_ptr->callback(NULL, 0, ptp_packet, 0, 0, 0, header_size, nx_receive_queue_ptr->context, NULL);
                            if (nx_status != NX_SUCCESS) error_handler();

                            /* Packet was consumed */
                            break;
                        }

                        /* Move to the next queue.  */
                        nx_receive_queue_ptr = nx_receive_queue_ptr->next_ptr;
                        if (nx_receive_queue_ptr == nx_ip_instance.nx_ip_interface[PRIMARY_INTERFACE].nx_interface_link_receive_queue_head) {

                            /* We have reached the end of the queue and found no callback */
                            nx_packet_release(ptp_packet);
                            ptp_event_counters.rx_client_not_found[port]++;
                            LOG_WARNING("PTP RX Couldn't route packet to client");
                            break;
                        }
                    }

                    break;
                }

                default: {
                    error_handler();
                    break;
                }
            }
        } else {
            error_handler();
        }
    }
}


void ptp_packet_insert_timestamp(NX_PACKET *packet_ptr, NX_PTP_TIME *time) {
    ((uint32_t *) packet_ptr->nx_packet_data_start)[0] = time->nanosecond;
    ((uint32_t *) packet_ptr->nx_packet_data_start)[1] = time->second_low;
    ((uint32_t *) packet_ptr->nx_packet_data_start)[2] = time->second_high;
}

void ptp_packet_extract_timestamp(NX_PACKET *packet_ptr, NX_PTP_TIME *time) {
    time->nanosecond  = ((uint32_t *) packet_ptr->nx_packet_data_start)[0];
    time->second_low  = ((uint32_t *) packet_ptr->nx_packet_data_start)[1];
    time->second_high = ((uint32_t *) packet_ptr->nx_packet_data_start)[2];
}


// TODO: Rx filter function.
//       should filter all PTP packets
//       should check VLANs (PTP_VLAN = put in queue, otherwise release packet)
