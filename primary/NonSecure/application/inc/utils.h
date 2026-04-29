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
#include "assert.h"

#include "tx_api.h"
#include "tx_thread.h"
#include "tx_initialize.h"

#include "app.h"
#include "config.h"


/* Because of the multiply, care should be taken when putting large values into these functions (>4,000,000) to make sure the don't overflow */
#define TICKS_TO_MS(ticks)        (((ticks) * 1000) / TX_TIMER_TICKS_PER_SECOND)
#define MS_TO_TICKS(ms)           (((ms) * TX_TIMER_TICKS_PER_SECOND) / 1000)

#define MIN(a, b)                 ((a) < (b) ? (a) : (b))
#define MAX(a, b)                 ((a) > (b) ? (a) : (b))
#define CONSTRAIN(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))


#define LOG_INFO(format, ...)                                         \
    ({                                                                \
        log_status_t _s = _LOG(LOG_TYPE_INFO, format, ##__VA_ARGS__); \
        if (_s != LOGGING_OK) error_handler();                        \
        _s;                                                           \
    })
#define LOG_INFO_NO_CHECK(format, ...) _LOG(LOG_TYPE_INFO, format, ##__VA_ARGS__)


#define LOG_WARNING(format, ...)                                         \
    ({                                                                   \
        log_status_t _s = _LOG(LOG_TYPE_WARNING, format, ##__VA_ARGS__); \
        if (_s != LOGGING_OK) error_handler();                           \
        _s;                                                              \
    })
#define LOG_WARNING_NO_CHECK(format, ...) _LOG(LOG_TYPE_WARNING, format, ##__VA_ARGS__)


#define LOG_ERROR(format, ...)                                         \
    ({                                                                 \
        log_status_t _s = _LOG(LOG_TYPE_ERROR, format, ##__VA_ARGS__); \
        if (_s != LOGGING_OK) error_handler();                         \
        _s;                                                            \
    })
#define LOG_ERROR_NO_CHECK(format, ...) _LOG(LOG_TYPE_ERROR, format, ##__VA_ARGS__)


/* Get the thread's logger and write the message using it */
#define _LOG(type, format, ...)                                                            \
    ({                                                                                     \
        if (UART_LOGGING_ENABLE) {                                                         \
            printf("(NS) %10lu: ", (unsigned long) HAL_GetTick());                         \
            printf(format "\n", ##__VA_ARGS__);                                            \
        }                                                                                  \
        log_write(get_logger(), (type), LOG_ESTIMATE_SIZE(format), format, ##__VA_ARGS__); \
    })


void write_mac_addr(uint8_t *buf);
bool compare_mac_addrs_with_mask(const uint8_t *addr1, const uint8_t *addr2, const uint8_t *mask);

uint32_t tx_thread_sleep_ms(uint32_t ms);
uint32_t tx_time_get_ms(void);
void     delay_ms(uint32_t ms);
void     systick_enable_pre_rtos(void);
void     delay_ns(uint32_t ns);

void set_3v3_regulator_to_fpwm(void);

log_handle_t *get_logger(void);
void          log_info(const char *format, ...);


#ifdef __cplusplus
}
#endif

#endif /* INC_UTILS_H_ */
