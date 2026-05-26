/*
 * ptp_init.h
 *
 *  Created on: May 15, 2026
 *      Author: bens1
 */

#ifndef INC_PTP_INIT_H_
#define INC_PTP_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "tx_app.h"
#include "nx_app.h"


extern const uint8_t ptp_dst_addr[MAC_ADDR_SIZE];

extern volatile uint32_t srcmeta_msw;
extern volatile uint32_t srcmeta_lsw;


tx_status_t ptp_start(void);
tx_status_t ptp_stop(void);

tx_status_t ptp_flush_packet_queue(TX_QUEUE *queue_ptr);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_INIT_H_ */
