/*
 * switch_error.c
 *
 *  Created on: May 15, 2026
 *      Author: bens1
 */

#include "app.h"
#include "switch.h"
#include "utils.h"


sja1105_status_t switch_reset(switch_index_t i) {

    sja1105_status_t status = SJA1105_OK;

    LOG_INFO("Resetting switch %d", i);

#if DEBUG
    error_handler();
#else
    // TODO: reset the switch
#endif

    return status;
}
