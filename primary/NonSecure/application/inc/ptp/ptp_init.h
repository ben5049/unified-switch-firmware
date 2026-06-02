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


#include "stdint.h"
#include "stdatomic.h"

#include "tx_app.h"
#include "nx_app.h"


extern volatile uint32_t ptp_srcmeta_msw, ptp_srcmeta_lsw;
extern volatile uint32_t ptp_mac_flt_msw, ptp_mac_flt_lsw;
extern volatile uint32_t ptp_mac_fltres_msw, ptp_mac_fltres_lsw;

extern volatile atomic_bool ptp_initialised;


tx_status_t ptp_start(void);
tx_status_t ptp_stop(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_INIT_H_ */
