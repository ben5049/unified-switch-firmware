/*
 * ptp_utils.h
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#ifndef INC_PTP_UTILS_H_
#define INC_PTP_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"

#include "tx_app.h"
#include "nx_app.h"


#define PTP_ETHERNET_ADDR_MSW         (0x0180)
#define PTP_ETHERNET_ADDR_LSW         (0xc200000e)
#define PTP_HEADER_PORT_OFFSET        (28)
#define PTP_HEADER_SEQUENCE_ID_OFFSET (30)

#define PTP_CLOCK_IDENTITY            MAC_ADDR_OCTET1, \
                           MAC_ADDR_OCTET2,            \
                           MAC_ADDR_OCTET3,            \
                           0xff,                       \
                           0xfe,                       \
                           MAC_ADDR_OCTET4,            \
                           MAC_ADDR_OCTET5,            \
                           MAC_ADDR_OCTET6

extern const uint8_t ptp_clock_identity[NX_PTP_CLOCK_IDENTITY_SIZE];
extern const uint8_t ptp_dst_addr[MAC_ADDR_SIZE];


void        ptp_packet_insert_timestamp(NX_PACKET *packet_ptr, const NX_PTP_TIME *time);
void        ptp_packet_extract_timestamp(const NX_PACKET *packet_ptr, NX_PTP_TIME *time);
nx_status_t ptp_packet_extract_port(const NX_PACKET *packet_ptr, uint32_t header_size, uint16_t *port);
nx_status_t ptp_packet_extract_sequence_id(const NX_PACKET *packet_ptr, uint32_t header_size, uint16_t *sequence_id);

void write_port_identity_eui(uint8_t *port_identity);
void write_port_identity_number(uint8_t *port_identity, uint16_t number);

void ptp_compute_offset(NX_PTP_TIME *t1, NX_PTP_TIME *t2, NX_PTP_TIME *t3, NX_PTP_TIME *t4, NX_PTP_TIME *offset);

void        ptp_mac_adjust_time_coarse(const NX_PTP_TIME *offset_time);
void        ptp_mac_set_addend(uint32_t addend);
void        ptp_mac_set_time(const NX_PTP_TIME *time_ptr);
void        ptp_mac_get_time(NX_PTP_TIME *time_ptr);
nx_status_t ptp_print_date(NX_PTP_TIME *time_ptr);

nx_status_t ptp_get_ingress_latency(port_index_t port, NX_PTP_TIME *latency);
nx_status_t ptp_get_egress_latency(port_index_t port, NX_PTP_TIME *latency);

tx_status_t ptp_flush_packet_queue(TX_QUEUE *queue_ptr);

void ptp_notify_port_down(port_index_t port);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_UTILS_H_ */
