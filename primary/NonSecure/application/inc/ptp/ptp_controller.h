/*
 * ptp_controller.h
 *
 *  Created on: June 6, 2026
 *      Author: bens1
 *
 *  PI Controller for syncing clocks
 */

#ifndef INC_PTP_CONTROLLER_H_
#define INC_PTP_CONTROLLER_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"

#include "nx_app.h"


typedef struct {

    bool initialised;

    float kp;
    float ki;

    int64_t error_integral;
    int64_t error_integral_max;

    uint32_t time_last;

} ptp_controller_t;


void    ptp_pi_controller_clear(ptp_controller_t *controller);
void    ptp_pi_controller_init(ptp_controller_t *controller, float kp, float ki, uint64_t i_max);
int32_t ptp_pi_controller_compute(ptp_controller_t *controller, int32_t error_current);


#ifdef __cplusplus
}
#endif

#endif /* INC_PTP_CONTROLLER_H_ */
