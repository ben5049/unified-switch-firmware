/*
 * podl.c
 *
 *  Created on: Jul 13, 2026
 *      Author: bens1
 */

#include "podl.h"
#include "secure_nsc.h"


bool podl_enable() {

    return s_start_podl_controller();

    // TODO: send config via UART
}
