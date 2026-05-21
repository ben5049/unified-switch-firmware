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
#include "switch_utils.h"


TX_THREAD ptp_rx_thread_handle;
uint8_t   ptp_rx_thread_stack[PTP_RX_THREAD_STACK_SIZE];

TX_QUEUE ptp_rx_packet_queue_handle;
uint32_t ptp_rx_packet_queue_stack[PTP_RX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];
TX_QUEUE ptp_rx_meta_queue_handle;
uint32_t ptp_rx_meta_queue_stack[PTP_RX_QUEUE_SIZE * PTP_MSG_SIZE_WORDS];


void ptp_rx_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status = TX_SUCCESS;

    ptp_event_info_t event_info;

    NX_PACKET  *ptp_packet;
    phy_index_t port;
    NX_PTP_TIME timestamp;

#if DEBUG
    uint32_t ptp_enqueued;
    uint32_t meta_enqueued;
    uint32_t packet_queue_high_water_mark = 0;
#endif

    while (1) {

        tx_status = tx_queue_receive(&ptp_rx_packet_queue_handle, &event_info, TX_WAIT_FOREVER);
#if DEBUG
        tx_queue_info_get(&ptp_rx_packet_queue_handle, NULL, &ptp_enqueued, NULL, NULL, NULL, NULL);
        if ((ptp_enqueued + 1) > packet_queue_high_water_mark) {
            packet_queue_high_water_mark = (ptp_enqueued + 1);
            LOG_INFO("PTP RX Queue high water mark = %lu/%d", packet_queue_high_water_mark, PTP_RX_QUEUE_SIZE);
        }
#endif
        if (tx_status == NX_SUCCESS) {

            switch (event_info.event) {

                case PTP_RX_EVENT_RECEIVE_PACKET: {

                    ptp_packet = event_info.data;

                    /* Get the follow up META frame */
                    tx_status = tx_queue_receive(&ptp_rx_meta_queue_handle, &event_info, MS_TO_TICKS(PTP_RX_TIMEOUT));
                    if (tx_status != TX_SUCCESS) error_handler();

                    // TODO: rethink since packets can be received and queued between checking sizes
#if DEBUG
                    /* Shouldn't be possible to have more META frames than PTP
                     * packets since META frames follow after each PTP packet */
                    tx_status = tx_queue_info_get(&ptp_rx_meta_queue_handle, NULL, &meta_enqueued, NULL, NULL, NULL, NULL);
                    if (tx_status != TX_SUCCESS) error_handler();
                    if (meta_enqueued > ptp_enqueued) error_handler();
#endif

                    /* Parse META frame for timestamp and port
                     * Note: This also frees the packet */
                    if (switch_parse_meta_frame(event_info.data, &port, &timestamp) != SJA1105_OK) error_handler();

                    // TODO: Pass to ptp client
                    //       Insert timestamp into packet
                    UNUSED(ptp_packet);

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
