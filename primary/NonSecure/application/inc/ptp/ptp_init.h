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


#include "nx_app.h"


nx_status_t ptp_configure(void);
void        ptp_set_ingress_correction(void);
void        ptp_set_egress_correction(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_INIT_H_ */
