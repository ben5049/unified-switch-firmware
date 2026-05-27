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
#include "ptp_thread.h"
#include "ptp_utils.h"


#define PTP_SYNC_PAYLOAD_LENGTH (44)


const uint8_t ptp_dst_addr[MAC_ADDR_SIZE] = {
    (uint8_t) (PTP_ETHERNET_ADDR_LSB),       /* Index 0: 0x0E */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 8),  /* Index 1: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 16), /* Index 2: 0x00 */
    (uint8_t) (PTP_ETHERNET_ADDR_LSB >> 24), /* Index 3: 0xC2 */
    (uint8_t) (PTP_ETHERNET_ADDR_MSB),       /* Index 4: 0x80 */
    (uint8_t) (PTP_ETHERNET_ADDR_MSB >> 8)   /* Index 5: 0x01 */
};


static uint8_t dummy_sync_payload[PTP_SYNC_PAYLOAD_LENGTH] = {
    0x00 | (NX_PTP_TRANSPORT_SPECIFIC_802 << 4),                        /* 0: MsgType = 0 (Sync), transport specific = 1 (gPTP) */
    0x02,                                                               /* 1: Version = 2 */
    0x00, PTP_SYNC_PAYLOAD_LENGTH,                                      /* 2-3: Message Length = 44 bytes */
    0x00, 0x00,                                                         /* 4-5: Domain Number = 0, Reserved */
    0x02, 0x00,                                                         /* 6-7: Flags (0x02 = Two-Step flag set) */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                     /* 8-15: Correction Field */
    0x00, 0x00, 0x00, 0x00,                                             /* 16-19: Reserved */
    MAC_ADDR_OCTET1, MAC_ADDR_OCTET2, MAC_ADDR_OCTET3, 0xff, 0xfe,
    MAC_ADDR_OCTET4, MAC_ADDR_OCTET5, MAC_ADDR_OCTET6, 0x00, NUM_PORTS, /* 20-29: Port Identity */
    0x00, 0x01,                                                         /* 30-31: Sequence ID */
    0x00,                                                               /* 32: Control Field (0x00 for Sync) */
    0x00,                                                               /* 33: Log Message Interval */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00          /* Sync-specific payload: Origin Timestamp (10 bytes) */
};


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


nx_status_t ptp_create_dummy_sync(NX_PACKET **packet_ptr_ptr) {

    nx_status_t status = NX_SUCCESS;
    NX_PACKET  *packet_ptr;

    /* Allocate a packet */
    status = nx_packet_allocate(&nx_small_packet_pool, packet_ptr_ptr, NX_PHYSICAL_HEADER, NX_NO_WAIT);
    if (status != NX_SUCCESS) goto end;

    packet_ptr = *packet_ptr_ptr;

    /* Append dummy payload to make the MAC see it as a PTP packet and timestamp it */
    status = nx_packet_data_append(packet_ptr, dummy_sync_payload, sizeof(dummy_sync_payload), &nx_small_packet_pool, NX_NO_WAIT);
    if (status != NX_SUCCESS) goto cleanup;

    /* Add Ethernet header */
    status = nx_link_ethernet_header_add(&nx_ip_instance, PRIMARY_INTERFACE, packet_ptr,
                                         PTP_ETHERNET_ADDR_MSB, PTP_ETHERNET_ADDR_LSB,
                                         NX_LINK_ETHERNET_PTP);
    if (status != NX_SUCCESS) goto cleanup;

    /* Enable egress timestamping for this packet */
    packet_ptr->nx_packet_interface_capability_flag |= NX_INTERFACE_CAPABILITY_PTP_TIMESTAMP;

    goto end;

cleanup:

    if (nx_packet_release(packet_ptr) != NX_SUCCESS) error_handler();

end:

    return status;
}


void ptp_compute_offset(NX_PTP_TIME *t1, NX_PTP_TIME *t2, NX_PTP_TIME *t3, NX_PTP_TIME *t4, NX_PTP_TIME *offset) {

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


void ptp_mac_adjust_time_coarse(NX_PTP_TIME *offset_time) {

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
void ptp_mac_set_time(NX_PTP_TIME *target_time) {

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
    time.Seconds     = target_time->second_low;
    time.NanoSeconds = target_time->nanosecond;

    /* Set the time */
    HAL_ETH_PTP_SetTime(&heth, &time);

    TX_RESTORE
}


/* Drain all items from a PTP queue and release any packets */
tx_status_t ptp_flush_packet_queue(TX_QUEUE *queue_ptr) {

    tx_status_t      status = TX_SUCCESS;
    ptp_event_info_t event_info;
    NX_PACKET       *packet;

    /* Loop until all packets released or error occured */
    do {

        /* Get an event */
        status = tx_queue_receive(queue_ptr, &event_info, TX_NO_WAIT);

        /* Ensure the data pointer actually contains a packet before releasing */
        if (status == TX_SUCCESS) {
            switch (event_info.event) {

                case PTP_TX_EVENT_SEND_PACKET:
                case PTP_RX_EVENT_RECEIVE_PACKET:
                    packet = event_info.packet_ptr;
                    if (nx_packet_release(packet) != NX_SUCCESS) {
                        status = TX_ERROR;
                        return status;
                    };
                    break;

                default:
                    break;
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

    } while (1);

    return status;
}
