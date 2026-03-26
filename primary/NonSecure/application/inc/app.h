/*
 * app_setup.h
 *
 *  Created on: 17 Aug 2025
 *      Author: bens1
 */

#ifndef INC_APP_SETUP_H_
#define INC_APP_SETUP_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"
#include "config.h"
#include "logging.h"


void mpu_config(void);
int  main(void);


extern log_handle_t  hlog_setup, hlog_generic, hlog_phy, hlog_sw, hlog_comms, hlog_system, hlog_network;
extern log_handle_t* loggers[];


#ifdef __cplusplus
}
#endif

#endif /* INC_APP_SETUP_H_ */
