/*
 * app.h
 *
 *  Created on: 17 Aug 2025
 *      Author: bens1
 */

#ifndef INC_APP_H_
#define INC_APP_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"
#include "config.h"
#include "constants.h"
#include "logging.h"
#include "error.h"


void periph_common_clock_config(void);
void mpu_config(void);
int  main(void);


extern log_handle_t  hlog_setup, hlog_generic, hlog_phy, hlog_sw, hlog_comms, hlog_system, hlog_network, hlog_ptp;
extern log_handle_t* loggers[NUM_LOGGERS];


#ifdef __cplusplus
}
#endif

#endif /* INC_APP_H_ */
