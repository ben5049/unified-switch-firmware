/*
 * sja1105_callbacks.c
 *
 *  Created on: Aug 5, 2025
 *      Author: bens1
 */

#include "stdatomic.h"
#include "stdarg.h"
#include "stdalign.h"

#include "tx_api.h"
#include "hal.h"

#include "app.h"
#include "utils.h"
#include "switch_thread.h"
#include "switch_callbacks.h"
#include "sja1105.h"


TX_MUTEX sja1105_mutex_handle;
alignas(32) static UCHAR switch0_byte_pool_buffer[SWITCH_MEM_POOL_SIZE];
static TX_BYTE_POOL switch0_byte_pool;
#if HW_VERSION == 5
alignas(32) static UCHAR switch1_byte_pool_buffer[SWITCH_MEM_POOL_SIZE];
static TX_BYTE_POOL switch1_byte_pool;
#endif


#if defined(DEBUG) && (HW_VERSION == 5)

/* Small state machine to check CRC calculations aren't interrupted */
typedef enum {
    CRC_OWNER_SW0 = SWITCH0,
    CRC_OWNER_SW1 = SWITCH1,
    CRC_OWNER_NONE
} crc_owner_t;
_Atomic(crc_owner_t) crc_owner = CRC_OWNER_NONE;

#endif


sja1105_status_t switch_byte_pool_init(uint8_t pool) {

    sja1105_status_t status = SJA1105_OK;

    /* Initialise pool 0 */
    if (pool & SWCH_POOL0) {
        if (tx_byte_pool_create(&switch0_byte_pool, "Switch 0 memory pool", switch0_byte_pool_buffer, SWITCH_MEM_POOL_SIZE) != TX_SUCCESS) {
            status = SJA1105_DYNAMIC_MEMORY_ERROR;
        }
    }

#if HW_VERSION == 5

    /* Initialise pool 1 */
    if (pool & SWCH_POOL1) {
        if (tx_byte_pool_create(&switch1_byte_pool, "Switch 1 memory pool", switch1_byte_pool_buffer, SWITCH_MEM_POOL_SIZE) != TX_SUCCESS) {
            status = SJA1105_DYNAMIC_MEMORY_ERROR;
        }
    }

#endif

    return status;
}

static void sja1105_write_cs_pin(sja1105_pinstate_t state, void *context) {

    switch_index_t switch_num = (switch_index_t) context;
    static bool    sw0_sel    = false;
    static bool    sw1_sel    = false;

    /* Select switch 0 */
    if (switch_num == SWITCH0) {
        if (state == SJA1105_PIN_RESET) {
            HAL_GPIO_WritePin(SWCH_CS0_GPIO_Port, SWCH_CS0_Pin, GPIO_PIN_RESET);
            sw0_sel = true;
        } else {
            HAL_GPIO_WritePin(SWCH_CS0_GPIO_Port, SWCH_CS0_Pin, GPIO_PIN_SET);
            sw0_sel = false;
        }
    }

#if HW_VERSION == 5

    /* Select switch 1 */
    else if (switch_num == SWITCH1) {
        if (state == SJA1105_PIN_RESET) {
            HAL_GPIO_WritePin(SWCH_CS1_GPIO_Port, SWCH_CS1_Pin, GPIO_PIN_RESET);
            sw1_sel = true;
        } else {
            HAL_GPIO_WritePin(SWCH_CS1_GPIO_Port, SWCH_CS1_Pin, GPIO_PIN_SET);
            sw1_sel = false;
        }
    }

#endif

    /* Invalid switch */
    else {
        error_handler();
    }

    /* Both switches shouldn't be selected at the same time */
#if DEBUG
    if (sw0_sel && sw1_sel) error_handler();
#endif
}

static sja1105_status_t sja1105_spi_transmit(const uint32_t *data, uint16_t size, uint32_t timeout, void *context) {

    sja1105_status_t status = SJA1105_OK;

    if (HAL_SPI_Transmit(&SWCH_SPI, (uint8_t *) data, size, timeout) != HAL_OK) {
        status = SJA1105_SPI_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_spi_receive(uint32_t *data, uint16_t size, uint32_t timeout, void *context) {

    sja1105_status_t status = SJA1105_OK;

    if (HAL_SPI_Receive(&SWCH_SPI, (uint8_t *) data, size, timeout) != HAL_OK) {
        status = SJA1105_SPI_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_spi_transmit_receive(const uint32_t *tx_data, uint32_t *rx_data, uint16_t size, uint32_t timeout, void *context) {

    sja1105_status_t status = SJA1105_OK;

    if (HAL_SPI_TransmitReceive(&SWCH_SPI, (uint8_t *) tx_data, (uint8_t *) rx_data, size, timeout) != HAL_OK) {
        status = SJA1105_SPI_ERROR;
    }

    return status;
}

static uint32_t sja1105_get_time_ms(void *context) {

    /* Use kernel time if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        return HAL_GetTick();
    } else {
        return tx_time_get_ms();
    }
}

static void sja1105_delay_ms(uint32_t ms, void *context) {

    /* Use kernel time if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        HAL_Delay(ms);
    } else {
        tx_thread_sleep_ms(ms);
    }
}

static void sja1105_delay_ns(uint32_t ns, void *context) {
    delay_ns(ns);
}

static sja1105_status_t sja1105_take_mutex(uint32_t timeout, void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't take the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    /* Take the mutex and work out the status */
    switch (tx_mutex_get(&sja1105_mutex_handle, MS_TO_TICKS(timeout))) {
        case TX_SUCCESS:
            status = SJA1105_OK;
            break;
        case TX_NOT_AVAILABLE:
            status = SJA1105_BUSY;
            break;
        default:
            status = SJA1105_MUTEX_ERROR;
            break;
    }

    return status;
}

static sja1105_status_t sja1105_give_mutex(void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't give the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_mutex_put(&sja1105_mutex_handle) != TX_SUCCESS) status = SJA1105_MUTEX_ERROR;

    return status;
}

static sja1105_status_t sja1105_allocate(uint32_t **memory_ptr, uint32_t size, void *context) {

    sja1105_status_t status     = SJA1105_OK;
    switch_index_t   switch_num = (switch_index_t) context;

    /* Allocate bytes from pool 0 */
    if (switch_num == SWITCH0) {
        if (tx_byte_allocate(&switch0_byte_pool, (void **) memory_ptr, size * sizeof(uint32_t), TX_NO_WAIT) != TX_SUCCESS) {
            status = SJA1105_DYNAMIC_MEMORY_ERROR;
        }
    }

#if HW_VERSION == 5

    /* Allocate bytes from pool 1 */
    else if (switch_num == SWITCH1) {
        if (tx_byte_allocate(&switch1_byte_pool, (void **) memory_ptr, size * sizeof(uint32_t), TX_NO_WAIT) != TX_SUCCESS) {
            status = SJA1105_DYNAMIC_MEMORY_ERROR;
        }
    }

#endif

    /* Invalid switch */
    else {
        status = SJA1105_PARAMETER_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_free(uint32_t *memory_ptr, void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Don't free memory if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_byte_release(memory_ptr) != TX_SUCCESS) {
        status = SJA1105_DYNAMIC_MEMORY_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_free_all(void *context) {

    sja1105_status_t status     = SJA1105_OK;
    switch_index_t   switch_num = (switch_index_t) context;

    /* Don't free memory if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    /* Reset byte pool 0 */
    if (switch_num == SWITCH0) {
        if (tx_byte_pool_delete(&switch0_byte_pool) == TX_SUCCESS) {
            status = switch_byte_pool_init(SWCH_POOL0);
        } else {
            status = SJA1105_DYNAMIC_MEMORY_ERROR;
        }
    }

#if HW_VERSION == 5

    /* Reset byte pool 1 */
    else if (switch_num == SWITCH1) {
        if (tx_byte_pool_delete(&switch1_byte_pool) == TX_SUCCESS) {
            status = switch_byte_pool_init(SWCH_POOL1);
        } else {
            status = SJA1105_DYNAMIC_MEMORY_ERROR;
        }
    }

#endif

    /* Invalid switch */
    else {
        status = SJA1105_PARAMETER_ERROR;
    }

    return status;
}

static sja1105_status_t sja1105_crc_reset(void *context) {

    sja1105_status_t status = SJA1105_OK;

    /* Reset the data register from the previous calculation */
    SWCH_CRC.Instance->CR |= CRC_CR_RESET;

    /* Make sure the initial value and polynomial are correct */
    SWCH_CRC.Instance->INIT = 0xffffffff;
    SWCH_CRC.Instance->POL  = 0x04c11db7;

#if defined(DEBUG) && (HW_VERSION == 5)
    atomic_store_explicit(&crc_owner, CRC_OWNER_NONE, memory_order_release);
#endif

    return status;
}

static sja1105_status_t sja1105_crc_accumulate(const uint32_t *buffer, uint32_t size, uint32_t *result, void *context) {

    sja1105_status_t status = SJA1105_OK;

#if defined(DEBUG) && (HW_VERSION == 5)

    /* Make sure the other switch isn't using the CRC */
    switch_index_t switch_num      = (switch_index_t) context;
    crc_owner_t    crc_owner_local = atomic_load_explicit(&crc_owner, memory_order_acquire);

    if (((switch_num == SWITCH0) && (crc_owner_local == CRC_OWNER_SW1)) ||
        ((switch_num == SWITCH1) && (crc_owner_local == CRC_OWNER_SW0))) {
        status = SJA1105_CRC_ERROR;
        LOG_ERROR("Switch %d attempted CRC accumulation without CRC ownership", (uint8_t) switch_num);
        return status;
    } else {
        atomic_store_explicit(&crc_owner, switch_num, memory_order_release);
    }

#endif

    *result = ~HAL_CRC_Accumulate(&SWCH_CRC, (uint32_t *) buffer, size * sizeof(uint32_t));

    return status;
}


const sja1105_callbacks_t sja1105_callbacks = {
    .callback_write_cs_pin         = &sja1105_write_cs_pin,
    .callback_spi_transmit         = &sja1105_spi_transmit,
    .callback_spi_receive          = &sja1105_spi_receive,
    .callback_spi_transmit_receive = &sja1105_spi_transmit_receive,
    .callback_get_time_ms          = &sja1105_get_time_ms,
    .callback_delay_ms             = &sja1105_delay_ms,
    .callback_delay_ns             = &sja1105_delay_ns,
    .callback_take_mutex           = &sja1105_take_mutex,
    .callback_give_mutex           = &sja1105_give_mutex,
    .callback_allocate             = &sja1105_allocate,
    .callback_free                 = &sja1105_free,
    .callback_free_all             = &sja1105_free_all,
    .callback_crc_reset            = &sja1105_crc_reset,
    .callback_crc_accumulate       = &sja1105_crc_accumulate,
    .callback_write_log            = &log_info,
};
