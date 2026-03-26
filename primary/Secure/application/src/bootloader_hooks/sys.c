/*
 * sys.c
 *
 *  Created on: Mar 26, 2026
 *      Author: bens1
 */

#include "hal.h"

#include "bootloader.h"


void bootloader_enable_irq() {
    __enable_irq();
}


void bootloader_disable_irq() {
    __disable_irq();
}