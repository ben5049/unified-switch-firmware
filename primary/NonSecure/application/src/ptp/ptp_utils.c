/*
 * ptp_utils.c
 *
 *  Created on: May 27, 2026
 *      Author: bens1
 */

#include "stdint.h"
#include "eth.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "ptp.h"


const uint8_t ptp_dst_addr[MAC_ADDR_SIZE] = {
    (uint8_t) (PTP_ETHERNET_ADDR_LSW),       /* Index 0: 0x0E */
    (uint8_t) (PTP_ETHERNET_ADDR_LSW >> 8),  /* Index 1: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSW >> 16), /* Index 2: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSW >> 24), /* Index 3: 0xC2 */
    (uint8_t) (PTP_ETHERNET_ADDR_MSW),       /* Index 4: 0x80 */
    (uint8_t) (PTP_ETHERNET_ADDR_MSW >> 8)   /* Index 5: 0x01 */
};


void ptp_packet_insert_timestamp(NX_PACKET *packet_ptr, const NX_PTP_TIME *time) {
    ((uint32_t *) packet_ptr->nx_packet_data_start)[0] = time->nanosecond;
    ((uint32_t *) packet_ptr->nx_packet_data_start)[1] = time->second_low;
    ((uint32_t *) packet_ptr->nx_packet_data_start)[2] = time->second_high;
}


void ptp_packet_extract_timestamp(const NX_PACKET *packet_ptr, NX_PTP_TIME *time) {
    time->nanosecond  = ((uint32_t *) packet_ptr->nx_packet_data_start)[0];
    time->second_low  = ((uint32_t *) packet_ptr->nx_packet_data_start)[1];
    time->second_high = ((uint32_t *) packet_ptr->nx_packet_data_start)[2];
}


/* Convert 48-bit MAC address to 64-bit EUI */
void write_port_identity_eui(uint8_t *port_identity) {
    port_identity[0] = MAC_ADDR_OCTET1;
    port_identity[1] = MAC_ADDR_OCTET2;
    port_identity[2] = MAC_ADDR_OCTET3;
    port_identity[3] = 0xff;
    port_identity[4] = 0xfe;
    port_identity[5] = MAC_ADDR_OCTET4;
    port_identity[6] = MAC_ADDR_OCTET5;
    port_identity[7] = MAC_ADDR_OCTET6;
}


void write_port_identity_number(uint8_t *port_identity, uint16_t number) {
    port_identity[8] = (uint8_t) ((number + 1) >> 8);
    port_identity[9] = (uint8_t) (number + 1); /* Indexed from 1 */
}


void ptp_compute_offset(const NX_PTP_TIME *t1, const NX_PTP_TIME *t2, const NX_PTP_TIME *t3, const NX_PTP_TIME *t4, NX_PTP_TIME *offset) {

    NX_PTP_TIME a;
    NX_PTP_TIME b;

    /* Compute A = t2 - t1 */
    _nx_ptp_client_utility_time_diff(t2, t1, &a);

    /* Compute B = t4 - t3 */
    _nx_ptp_client_utility_time_diff(t4, t3, &b);

    /* Compute offset = (B - A) / 2 */

    _nx_ptp_client_utility_time_diff(&b, &a, offset);
    _nx_ptp_client_utility_time_div_by_2(offset);
}


void ptp_mac_adjust_time_coarse(const NX_PTP_TIME *offset_time) {

    TX_INTERRUPT_SAVE_AREA

    ETH_TimeTypeDef time;

    assert(offset_time->second_high == 0);
    assert(offset_time->second_low == 0);

    TX_DISABLE

    time.Seconds = 0;
    if (offset_time->nanosecond > 0) {
        time.NanoSeconds = offset_time->nanosecond;
        HAL_ETH_PTP_AddTimeOffset(&heth, HAL_ETH_PTP_NEGATIVE_UPDATE, &time);
    } else {
        time.NanoSeconds = -offset_time->nanosecond;
        HAL_ETH_PTP_AddTimeOffset(&heth, HAL_ETH_PTP_POSITIVE_UPDATE, &time);
    }

    TX_RESTORE
}


/* Set the STM32 MAC timestamp counter */
void ptp_mac_set_time(const NX_PTP_TIME *time_ptr) {

    TX_INTERRUPT_SAVE_AREA

    uint32_t        tickstart;
    ETH_TimeTypeDef time;

    TX_DISABLE

    /* Wait for MAC to be ready */
    tickstart = HAL_GetTick();
    while (__HAL_ETH_GET_PTP_CONTROL(&heth, ETH_MACTSCR_TSUPDT) != 0) {
        if ((HAL_GetTick() - tickstart) > 100) {
            error_handler();
            return;
        }
    }

    /* Format time */
    time.Seconds     = time_ptr->second_low;
    time.NanoSeconds = time_ptr->nanosecond;

    /* Set the time */
    HAL_ETH_PTP_SetTime(&heth, &time);

    TX_RESTORE
}


void ptp_mac_get_time(NX_PTP_TIME *time_ptr) {

    TX_INTERRUPT_SAVE_AREA

    ETH_TimeTypeDef time;
    uint32_t        sec1;
    uint32_t        sec2;
    uint32_t        ns;

    TX_DISABLE

    HAL_ETH_PTP_GetTime(&heth, &time);
    sec1 = time.Seconds;
    ns   = time.NanoSeconds;
    HAL_ETH_PTP_GetTime(&heth, &time);
    sec2                  = time.Seconds;
    time_ptr->second_high = 0;

    /* The offset standard deviation is below 50 ns */
    time_ptr->second_low = ns < 500000000UL ? sec2 : sec1;
    time_ptr->nanosecond = (LONG) ns;

    TX_RESTORE
}


/* Drain all items from a PTP queue and release any packets */
tx_status_t ptp_flush_packet_queue(TX_QUEUE *queue_ptr) {

    tx_status_t             status = TX_SUCCESS;
    ptp_packet_event_info_t event_info;
    ptp_event_t             event;
    NX_PACKET              *packet;

    assert(queue_ptr->tx_queue_message_size == PTP_PACKET_MSG_SIZE_WORDS);

    /* Loop until all packets released or error occured */
    do {

        /* Get an event */
        status = tx_queue_receive(queue_ptr, &event_info, TX_NO_WAIT);

        /* Ensure the data pointer actually contains a packet before releasing */
        if (status == TX_SUCCESS) {

            event  = event_info.event;
            packet = event_info.packet_ptr;

            assert((event == PTP_TX_EVENT_SEND_PACKET) ||
                   (event == PTP_RX_EVENT_RECEIVE_PTP_PACKET) ||
                   (event == PTP_RX_EVENT_RECEIVE_META_FRAME));

            NX_CHECK(nx_packet_release(packet));
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

    } while (1);

    return status;
}


void ptp_notify_port_down(port_index_t port) {

    tx_status_t             status = TX_SUCCESS;
    ptp_client_event_info_t event_info;

    assert(port < NUM_PHYS);

    event_info.event = PTP_CLIENT_EVENT_TIMEOUT;
    event_info.port  = port;
    status           = ptp_client_event_queue_send(&event_info);
    TX_CHECK(status);
}
