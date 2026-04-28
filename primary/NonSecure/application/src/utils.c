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


void delay_ns(uint32_t ns) {

    uint32_t ticks;
    uint32_t periods;

    /* Calculate the number of CPU cycles required */
    /* (ns * CPU_MHz) / 1000 = cycles */
    /* Integer division rounds down, but the function overhead ensures that a delays of 0 cycles never happens */
    ticks = ((ns * (SystemCoreClock / 1000000)) / 1000);

    /* Use a NOP loop for shorter delays */
    if (ticks < 50) {

        /* An unrolled while loop */
        while (ticks > 4) {
            __NOP();
            __NOP();
            __NOP();
            __NOP();
            ticks -= 4;
        }

        /* Catch the remainder */
        while (ticks > 0) {
            __NOP();
            ticks--;
        }
    }

    /* Use TIM7 for longer delays */
    else {

        /* Setup */
        SET_BIT(TIM7->CR1, TIM_CR1_OPM);     /* Enable one-pulse mode */
        CLEAR_BIT(TIM7->CR1, TIM_CR1_URS);   /* Set the update request source to any */
        TIM7->ARR = (ticks % (1 << 16)) - 1; /* Set the first threshold */
        SET_BIT(TIM7->EGR, TIM_EGR_UG);      /* Re-initialise timer counter */
        CLEAR_BIT(TIM7->SR, TIM_SR_UIF);     /* Clear update flag */
        SET_BIT(TIM7->CR1, TIM_CR1_CEN);     /* Enable counting */

        /* Count the number of full periods that must be elapsed (other than the one running now) */
        periods = ticks / (1 << 16);

        /* Wait for the update flag */
        while (!(TIM7->SR & TIM_SR_UIF)) {
            __NOP();
        }

        /* Wait for the remaining periods */
        if (periods) {

            /* Setup */
            TIM7->ARR = (1 << 16) - 1;

            for (uint_fast32_t i = 0; i < periods; i++) {

                /* Start again */
                SET_BIT(TIM7->EGR, TIM_EGR_UG); /* Re-initialise timer counter */
                CLEAR_BIT(TIM7->SR, TIM_SR_UIF);
                SET_BIT(TIM7->CR1, TIM_CR1_CEN);

                /* Wait for the update flag */
                while (!(TIM7->SR & TIM_SR_UIF)) {
                    __NOP();
                }
            }
        }
    }
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
