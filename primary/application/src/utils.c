/*
 * utils.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "main.h"

#include "utils.h"
#include "config.h"
#include "sja1105.h"
#include "secure_nsc.h"


/* This function should be called in MX_ETH_Init */
void write_mac_addr(uint8_t* buf) {
    buf[0] = MAC_ADDR_OCTET1;
    buf[1] = MAC_ADDR_OCTET2;
    buf[2] = MAC_ADDR_OCTET3;
    buf[3] = MAC_ADDR_OCTET4;
    buf[4] = MAC_ADDR_OCTET5;
    buf[5] = MAC_ADDR_OCTET6;
}


bool compare_mac_addrs_with_mask(const uint8_t* addr1, const uint8_t* addr2, const uint8_t* mask) {
    for (uint_fast8_t i = 0; i < MAC_ADDR_SIZE; i++) {
        if ((addr1[i] & mask[i]) != (addr2[i] & mask[i])) return false;
    }
    return true;
}


uint32_t tx_thread_sleep_ms(uint32_t ms) {
    return tx_thread_sleep((uint32_t) MS_TO_TICKS((uint64_t) ms)); /* Cast to 64-bit uint to prevent premature overflow */
}


uint32_t tx_time_get_ms() {
    return (uint32_t) TICKS_TO_MS((uint64_t) tx_time_get()); /* Cast to 64-bit uint to prevent premature overflow */
}


void delay_ns(uint32_t ns) {

    /* CPU runs at 250MHz so one instruction is 4ns.
     * The loop contains a NOP, ADDS, CMP and branch instruction per cycle.
     * This means the loop delay is 4 * 4ns = 16ns.
     * This is true for O3 but will take longer for O0.
     */
    for (uint32_t t = 0; t < ns; t += 16) {
        __NOP();
    }
}


void set_3v3_regulator_to_FPWM() {
    __disable_irq();
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, RESET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, SET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, RESET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, SET);
    __enable_irq();
}


/* This function can only be called by threads with a secure stack allocated, or before the scheduler starts */
void ns_log_write(const char* format, ...) {
    va_list args;
    va_start(args, format);
    s_log_vwrite(format, args);
    va_end(args);
}
