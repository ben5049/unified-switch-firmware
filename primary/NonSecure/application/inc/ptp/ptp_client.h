/*
 * ptp_client.h
 *
 *  Created on: June 4, 2026
 *      Author: bens1
 */

#ifndef INC_PTP_CLIENT_H_
#define INC_PTP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"

#include "tx_app.h"
#include "nx_app.h"
#include "phy_thread.h"


extern NX_PTP_CLIENT ptp_client[NUM_PHYS];
extern TX_MUTEX      ptp_client_event_queue_mutex_handle;


nx_status_t ptp_client_start(port_index_t i, uint8_t *port_identity);
nx_status_t ptp_client_reset(port_index_t i, bool master);
nx_status_t ptp_client_restart_all(void);

tx_status_t ptp_client_filter_event_queue(port_bitset_t keep_ports, port_bitset_t remove_ports);
tx_status_t ptp_client_event_queue_send(ptp_client_event_info_t *event_info);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_CLIENT_H_ */
