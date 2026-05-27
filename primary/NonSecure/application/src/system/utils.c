/*
 * utils.c
 *
 *  Created on: Jul 28, 2025
 *      Author: bens1
 */

#include "app.h"
#include "utils.h"
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


void delay_ms(uint32_t ms) {

    /* Use kernel time if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        HAL_Delay(ms);
    } else {
        tx_thread_sleep_ms(ms);
    }
}


/* Required to use delay_ns before the rtos has started */
void systick_enable_pre_rtos() {
    SysTick->LOAD = 0x00ffffff;                                           /* Max value */
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; /* Enable SysTick with processor clock, but keep TICKINT disabled to prevent jumping to RTOS interrupt handler */
}

/* Accurate to within 1us (at lowest optimisation level) */
void delay_ns(uint32_t ns) {

    TX_INTERRUPT_SAVE_AREA;

    uint32_t required_ticks;
    uint32_t elapsed_ticks;
    uint32_t start_val;
    uint32_t load_val;
    uint32_t current_val;
    uint16_t ms;

    /* Capture start_val right at the start so the delay includes the calculation of the number of ticks to delay for */
    start_val = SysTick->VAL;

    /* For large delays use delay ms */
    ms = ns / 1000000;
    if (ms) {
        delay_ms(ms);
        ns        -= ms * 1000000;
        start_val  = SysTick->VAL;
    }

    /* Calculate the number of CPU cycles required */
    /* (ns * CPU_MHz) / 1000 = cycles */
    required_ticks = (uint32_t) (((uint64_t) ns * (SystemCoreClock / 1000000)) / 1000);
    if (required_ticks == 0) return;

    load_val      = SysTick->LOAD;
    elapsed_ticks = 0;

    /* Disable RTOS for accurate timing */
    TX_DISABLE;

    /* Poll until delay complete */
    while (elapsed_ticks < required_ticks) {

        current_val = SysTick->VAL;

        /* Normally counting down */
        if (current_val <= start_val) {
            elapsed_ticks += (start_val - current_val);
        }

        /* Counter hit 0 and was reloaded */
        else {
            elapsed_ticks += (start_val + load_val - current_val + 1);
        }

        start_val = current_val;
    }

    TX_RESTORE;
}


void set_3v3_regulator_to_fpwm() {
    __disable_irq();
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, GPIO_PIN_RESET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, GPIO_PIN_SET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, GPIO_PIN_RESET);
    delay_ns(500);
    HAL_GPIO_WritePin(MODE_3V3_GPIO_Port, MODE_3V3_Pin, GPIO_PIN_SET);
    __enable_irq();
}


/* Get the appropriate logger handle */
log_handle_t* get_logger() {
    TX_THREAD*    thread_ptr    = tx_thread_identify();
    log_handle_t* logger_handle = NULL;

    /* Not in a thread */
    if (thread_ptr == TX_NULL) {

        /* Kernel not started */
        if (_tx_thread_system_state != TX_INITIALIZE_IS_FINISHED) {
            logger_handle = &hlog_setup;
        }

        /* In an ISR */
        else {
            logger_handle = &hlog_generic;
        }
    }

    /* In a thread */
    else {

        /* User created thread. Note: we can't trust thread_ptr->logger isn't from uninitialised memory */
        for (uint_fast8_t i = 0; (logger_handle == NULL) && (i < NUM_LOGGERS); i++) {
            if (thread_ptr->logger == loggers[i]) logger_handle = thread_ptr->logger;
        }

        /* System created thread */
        if (logger_handle == NULL) {
            logger_handle = &hlog_system;
        }
    }
    return logger_handle;
}

/* This function writes a log.
   Note: It is not preferred since it cannot estimate the log size (max 128 bytes) and is always
         LOG_TYPE_INFO. LOG_x() Macros should be used instead where possible.  */
void log_info(const char* format, ...) {

    log_status_t status = LOGGING_OK;

    va_list args;
    va_start(args, format);

    status = log_vwrite(get_logger(), LOG_TYPE_INFO, 128, format, args);
    if (status != LOGGING_OK) error_handler();

    va_end(args);
}
