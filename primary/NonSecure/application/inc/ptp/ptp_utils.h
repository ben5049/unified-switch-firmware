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


extern const uint8_t ptp_dst_addr[MAC_ADDR_SIZE];


void ptp_packet_insert_timestamp(NX_PACKET *packet_ptr, const NX_PTP_TIME *time);
void ptp_packet_extract_timestamp(const NX_PACKET *packet_ptr, NX_PTP_TIME *time);

void write_port_identity_eui(uint8_t *port_identity);
void write_port_identity_number(uint8_t *port_identity, uint16_t number);

void ptp_compute_offset(const NX_PTP_TIME *t1, const NX_PTP_TIME *t2, const NX_PTP_TIME *t3, const NX_PTP_TIME *t4, NX_PTP_TIME *offset);

void ptp_mac_adjust_time_coarse(const NX_PTP_TIME *offset_time);
void ptp_mac_set_time(const NX_PTP_TIME *time_ptr);
void ptp_mac_get_time(NX_PTP_TIME *time_ptr);

tx_status_t ptp_flush_packet_queue(TX_QUEUE *queue_ptr);

void ptp_notify_port_down(port_index_t port);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_UTILS_H_ */
