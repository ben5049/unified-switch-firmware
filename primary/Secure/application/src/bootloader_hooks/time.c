/*
 * time.c
 *
 *  Created on: Mar 26, 2026
 *      Author: bens1
 */

#include "hal.h"

#include "bootloader.h"


uint32_t bootloader_get_time_ms() {
    return HAL_GetTick();
}


void bootloader_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}


void bootloader_enable_tick() {
    HAL_ResumeTick();
}


void bootloader_disable_tick() {
    HAL_SuspendTick();
}
