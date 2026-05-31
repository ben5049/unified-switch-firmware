/*
 * ptp_sync.h
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#ifndef INC_PTP_SYNC_H_
#define INC_PTP_SYNC_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "nx_app.h"


nx_status_t ptp_mac_sync(void);
nx_status_t ptp_switch_sync(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_SYNC_H_ */
