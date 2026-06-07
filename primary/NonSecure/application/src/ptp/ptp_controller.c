/*
 * ptp_controller.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"

#include "tx_app.h"
#include "ptp.h"
#include "utils.h"


void ptp_pi_controller_clear(ptp_controller_t *controller, uint32_t time_current) {

    controller->error_integral = 0;
    controller->time_last      = time_current;
}


void ptp_pi_controller_init(ptp_controller_t *controller, float kp, float ki, uint64_t i_max, uint32_t time_start) {

    controller->kp                 = kp;
    controller->ki                 = ki;
    controller->error_integral     = 0;
    controller->error_integral_max = i_max;
    controller->time_last          = time_start;
    controller->initialised        = true;
}


int32_t ptp_pi_controller_compute(ptp_controller_t *controller, int32_t error_current, uint32_t time_current) {

    /* Get the error integral */
    controller->error_integral += (((int64_t) error_current) *
                                   ((int64_t) (time_current - controller->time_last))) /
                                  1000;
    controller->error_integral = CONSTRAIN(controller->error_integral,
                                           -controller->error_integral_max,
                                           controller->error_integral_max);

    /* Save the time for next compute */
    controller->time_last = time_current;

    /* Calculate the controller output */
    return (int32_t) (((int64_t) error_current * controller->kp) +
                      (controller->error_integral * controller->ki));
}
