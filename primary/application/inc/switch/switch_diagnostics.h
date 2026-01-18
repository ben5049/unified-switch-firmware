/*
 * switch_diagnostics.h
 *
 *  Created on: Oct 4, 2025
 *      Author: bens1
 */

#ifndef INC_SWITCH_SWITCH_DIAGNOSTICS_H_
#define INC_SWITCH_SWITCH_DIAGNOSTICS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "sja1105.h"


sja1105_status_t init_switch_diagnostics(void);
sja1105_status_t publish_switch_diagnostics(uint32_t current_time);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_SWITCH_DIAGNOSTICS_H_ */
