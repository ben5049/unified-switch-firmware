/*
 * utils.h
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"
#include "stdarg.h"
#include "tx_api.h"


/* Because of the multiply, care should be taken when putting large values into these functions (>4,000,000) to make sure the don't overflow */
#define TICKS_TO_MS(ticks)        (((ticks) * 1000) / TX_TIMER_TICKS_PER_SECOND)
#define MS_TO_TICKS(ms)           (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)

#define MIN(a, b)                 ((a) < (b) ? (a) : (b))
#define MAX(a, b)                 ((a) > (b) ? (a) : (b))
#define CONSTRAIN(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

void write_mac_addr(uint8_t* buf);
bool compare_mac_addrs_with_mask(const uint8_t* addr1, const uint8_t* addr2, const uint8_t* mask);

uint32_t tx_thread_sleep_ms(uint32_t ms);
uint32_t tx_time_get_ms();

void delay_ns(uint32_t ns);

void set_3v3_regulator_to_FPWM(void);

void ns_log_write(const char* format, ...);


#ifdef __cplusplus
}
#endif

#endif /* INC_UTILS_H_ */
