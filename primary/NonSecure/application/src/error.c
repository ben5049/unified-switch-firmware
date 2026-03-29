/*
 * error.c
 *
 *  Created on: Mar 28, 2026
 *      Author: bens1
 */

#include "main.h"

#include "secure_nsc.h"


void error_handler() {

    __disable_irq();

    // TODO: Handle errors

#if DEBUG
    while (1);
#endif

    /* Call the secure error handler if no solution could be found
     * This will log the crash and reboot the device. Potentially
     * using an older (stable) version of the firmware if one can
     * be found. */
    s_error_handler();

    __enable_irq();
}
