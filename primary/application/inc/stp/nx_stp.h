/*
 * stp_extra.h
 *
 *  Created on: Aug 3, 2025
 *      Author: bens1
 *
 * Wrapper around netx functions for sending and receiving BPDUs.
 */

#ifndef INC_STP_NX_STP_H_
#define INC_STP_NX_STP_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdbool.h"
#include "nx_api.h"
#include "nx_app.h"
#include "hal.h"

#include "switch_thread.h"


#define BPDU_BPDU_MAX_BUFFER_SIZE   (128)
#define BPDU_DST_ADDR_SIZE          (6)
#define BPDU_SRC_ADDR_SIZE          (6)
#define BPDU_SIZE_OR_ETHERTYPE_SIZE (2)
#define BPDU_LLC_SIZE               (3)
#define BPDU_HEADER_SIZE            (BPDU_DST_ADDR_SIZE + BPDU_SRC_ADDR_SIZE + BPDU_SIZE_OR_ETHERTYPE_SIZE + BPDU_LLC_SIZE)


typedef struct {
    NX_PACKET            *tx_packet_ptr;
    NX_IP                *ip_ptr;
    NX_PACKET            *rx_packet_queue_head;
    NX_PACKET            *rx_packet_queue_tail;
    TX_EVENT_FLAGS_GROUP *events;
} nx_stp_t;


extern nx_stp_t      nx_stp;
extern const uint8_t bpdu_dest_address[BPDU_DST_ADDR_SIZE];
extern const uint8_t bpdu_dest_address_mask[BPDU_DST_ADDR_SIZE];
extern const uint8_t bpdu_llc[BPDU_LLC_SIZE];

nx_status_t nx_stp_init(NX_IP *ip_ptr, char *name, TX_EVENT_FLAGS_GROUP *events);
nx_status_t nx_stp_allocate_packet(void);
nx_status_t nx_stp_send_packet(void);
nx_status_t nx_stp_packet_deferred_receive(NX_IP *ip_ptr, NX_PACKET *packet_ptr);

#ifdef __cplusplus
}
#endif

#endif /* INC_STP_NX_STP_H_ */
