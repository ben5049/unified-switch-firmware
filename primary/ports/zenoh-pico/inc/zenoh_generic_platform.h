/*
 * zenoh_config.h
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#ifndef INC_ZENOH_GENERIC_PLATFORM_H_
#define INC_ZENOH_GENERIC_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdbool.h"
#include "stdint.h"

#include "zenoh-pico/config.h"
#include "hal.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nxd_ptp_client.h"


#ifndef Z_TASK_STACK_SIZE
#define Z_TASK_STACK_SIZE 4096
#endif

#ifndef Z_TASK_PRIORITY
#define Z_TASK_PRIORITY 14
#endif

#ifndef Z_TASK_PREEMPT_THRESHOLD
#define Z_TASK_PREEMPT_THRESHOLD 14
#endif

#ifndef Z_TASK_TIME_SLICE
#define Z_TASK_TIME_SLICE 1
#endif

/*
 * Use this define if you want to use functions HAL_UARTEx_RxEventCallback and
 * HAL_UART_ErrorCallback in your application with multiple other uart ports.
 * Application is then responisble for calling zptxstm32_rx_event_cb and zptxstm32_error_event_cb functions
 * inside these two HAL interrupts.
 */
#ifndef ZENOH_THREADX_STM32_GEN_IRQ
#define ZENOH_THREADX_STM32_GEN_IRQ 1
#endif

typedef struct {
    TX_THREAD threadx_thread;
    uint8_t   threadx_stack[Z_TASK_STACK_SIZE];
} _z_task_t;

typedef void *z_task_attr_t; // Not used

typedef TX_MUTEX _z_mutex_rec_t;
typedef TX_MUTEX _z_mutex_t;
typedef struct {
    TX_MUTEX     mutex;
    TX_SEMAPHORE sem;
    UINT         waiters;
} _z_condvar_t;

typedef NX_PTP_TIME z_clock_t;
typedef ULONG       z_time_t;

#if Z_FEATURE_LINK_SERIAL == 1
#ifndef ZENOH_HUART
#error Please define ZENOH_HUART: STM32CubeMX - Connectivity – USARTx – User constants, click “Add IP Handle Label” and enter
#error ZENOH_HUART for a serial port which will be used with zenoh-pico
#error Alternatively, define ZENOH_HUART by hand, set to huart1, huart2, etc.
#endif
#endif


typedef struct {
    bool     _err;
    uint32_t timeout;
    union {
#if Z_FEATURE_LINK_TCP == 1
        NX_TCP_SOCKET *tcp_socket;
#endif
#if Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
        NX_UDP_SOCKET *udp_socket;
#endif
#if Z_FEATURE_LINK_SERIAL == 1
        BufferedSerial *_serial;
#endif
    };
} _z_sys_net_socket_t;


typedef struct {
    bool _err;
#if Z_FEATURE_LINK_TCP == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1 || Z_FEATURE_LINK_UDP_UNICAST == 1
    uint32_t ip_address;
    uint16_t port;
#endif
} _z_sys_net_endpoint_t;


#if ZENOH_THREADX_STM32_GEN_IRQ == 0
void zptxstm32_rx_event_cb(UART_HandleTypeDef *huart, uint16_t offset);
void zptxstm32_error_event_cb(UART_HandleTypeDef *huart);
#endif


void z_free_with_context(void *data, void *context);


#ifdef __cplusplus
}
#endif

#endif /* INC_ZENOH_GENERIC_PLATFORM_H_ */
