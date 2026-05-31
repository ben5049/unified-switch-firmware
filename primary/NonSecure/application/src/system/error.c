/*
 * error.c
 *
 *  Created on: Mar 28, 2026
 *      Author: bens1
 */

#include "main.h"

#include "error.h"
#include "utils.h"
#include "secure_nsc.h"


void error_handler() {

    __disable_irq();

    // TODO: Handle errors

    /* Call the secure error handler if no solution could be found
     * This will log the crash and reboot the device. Potentially
     * using an older (stable) version of the firmware if one can
     * be found. */
    s_error_handler();

    __enable_irq();
}


void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth) {

    if (heth->ErrorCode & HAL_ETH_ERROR_PARAM) {
        error_handler();
    }
    if (heth->ErrorCode & HAL_ETH_ERROR_BUSY) {
        error_handler();
    }
    if (heth->ErrorCode & HAL_ETH_ERROR_TIMEOUT) {
        error_handler();
    }
    if (heth->ErrorCode & HAL_ETH_ERROR_DMA) {
        if (heth->DMAErrorCode & ETH_DMA_RX_NO_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_RX_DESC_READ_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_RX_DESC_WRITE_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_RX_BUFFER_READ_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_RX_BUFFER_WRITE_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_TX_NO_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_TX_DESC_READ_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_TX_DESC_WRITE_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_TX_BUFFER_READ_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_TX_BUFFER_WRITE_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_CONTEXT_DESC_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_FATAL_BUS_ERROR_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_EARLY_TX_IT_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_RX_WATCHDOG_TIMEOUT_FLAG) {
            error_handler();
        }
        if (heth->DMAErrorCode & ETH_DMA_RX_PROCESS_STOPPED_FLAG) {
            error_handler();
        }
        /* When debugging ignore this error since it can be caused by hitting a breakpoint */
#if !DEBUG
        if (heth->DMAErrorCode & ETH_DMA_RX_BUFFER_UNAVAILABLE_FLAG) {
            error_handler();
        }
#endif
        if (heth->DMAErrorCode & ETH_DMA_TX_PROCESS_STOPPED_FLAG) {
            error_handler();
        }
    }
    if (heth->ErrorCode & HAL_ETH_ERROR_MAC) {
    }
#if (USE_HAL_ETH_REGISTER_CALLBACKS == 1)
    if (heth->ErrorCode & HAL_ETH_ERROR_INVALID_CALLBACK) {
        error_handler();
    }
#endif
}


void thread_stack_error_handler(TX_THREAD *thread_ptr) {
    error_handler();
}
