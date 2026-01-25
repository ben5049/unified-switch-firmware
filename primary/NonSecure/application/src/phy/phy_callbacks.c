/*
 * phy_callbacks.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "hal.h"
#include "tx_api.h"
#include "main.h"

#include "88q211x.h"
#include "lan867x.h"
#include "phy_callbacks.h"
#include "phy_thread.h"
#include "stp_thread.h"
#include "utils.h"
#include "tx_app.h"
#include "phy_platform.h"


TX_MUTEX             phy_mutex_handle;
TX_EVENT_FLAGS_GROUP phy_events_handle;


/* 88Q2112 PHY MDIO lines are selected by an external mux */
static phy_status_t select_phy(phy_index_t phy_num) {

    phy_status_t   status       = PHY_OK;
    static uint8_t prev_phy_num = NUM_PHYS;

    /* Mux already has the correct PHY selected */
    if (phy_num == prev_phy_num) return status;

    /* Mux selection */
    switch (phy_num) {

        /* Channel Y5 */
        case PHY1_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_SET);
            break;

        /* Channel Y4 */
        case PHY2_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_SET);
            break;

        /* Channel Y2 */
        case PHY3_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_RESET);
            break;

        /* Channel Y1 */
        case PHY4_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_RESET);
            break;

        /* Channel Y3 */
        case PHY5_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_RESET);
            break;

        default:
            status = PHY_PARAMETER_ERROR; /* Only 88Q2112 PHYs are MUX'd */
            break;
    }

    /* Delay while mux switches over and pullup resistors take effect */
    delay_ns(200);
    prev_phy_num = phy_num;

    return status;
}


static phy_status_t phy_88q2112_callback_read_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    phy_status_t status = PHY_OK;

    /* Select the PHY */
    status = select_phy((phy_index_t) context);
    PHY_CHECK_RET(status);

    /* 88Q2112 only needs 1 preamble bit */
    /* Set the clock frequency to 9.62MHz (PHY supports up to 12.5MHz) */
    status = phy_read_reg_c45(phy_addr, mmd_addr, reg_addr, data, timeout, true, ETH_MACMDIOAR_CR_DIV26);

    return status;
}

static phy_status_t phy_lan8671_callback_read_reg(uint8_t phy_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 2.45MHz (PHY supports up to 4MHz) */
    return phy_read_reg_c22(phy_addr, reg_addr, data, timeout, ETH_MACMDIOAR_CR_DIV102);
}

static phy_status_t phy_dp83867_callback_read_reg(uint8_t phy_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 15.6MHz (PHY supports up to 25MHz) */
    return phy_read_reg_c22(phy_addr, reg_addr, data, timeout, ETH_MACMDIOAR_CR_DIV16);
}


static phy_status_t phy_88q2112_callback_write_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    phy_status_t status = PHY_OK;

    /* Select the PHY */
    status = select_phy((phy_index_t) context);
    PHY_CHECK_RET(status);

    /* 88Q2112 only needs 1 preamble bit */
    /* Set the clock frequency to 9.62MHz (PHY supports up to 12.5MHz) */
    status = phy_write_reg_c45(phy_addr, mmd_addr, reg_addr, data, timeout, true, ETH_MACMDIOAR_CR_DIV26);

    return status;
}

static phy_status_t phy_lan8671_callback_write_reg(uint8_t phy_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 2.45MHz (PHY supports up to 4MHz) */
    return phy_write_reg_c22(phy_addr, reg_addr, data, timeout, ETH_MACMDIOAR_CR_DIV102);
}

static phy_status_t phy_dp83867_callback_write_reg(uint8_t phy_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 15.6MHz (PHY supports up to 25MHz) */
    return phy_write_reg_c22(phy_addr, reg_addr, data, timeout, ETH_MACMDIOAR_CR_DIV16);
}


static uint32_t phy_callback_get_time_ms(void *context) {

    /* Use kernel time only if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        return HAL_GetTick();
    } else {
        return tx_time_get_ms();
    }
}


static void phy_callback_delay_ms(uint32_t ms, void *context) {

    /* Use kernel time only if it has been started */
    if (tx_thread_identify() == TX_NULL) {
        HAL_Delay(ms);
    } else {
        tx_thread_sleep_ms(ms);
    }
}


static void phy_callback_delay_ns(uint32_t ns, void *context) {
    delay_ns(ns);
}


static phy_status_t phy_callback_take_mutex(uint32_t timeout, void *context) {

    phy_status_t status = PHY_OK;

    /* Don't take the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    /* Take the mutex and work out the status */
    switch (tx_mutex_get(&phy_mutex_handle, MS_TO_TICKS(timeout))) {
        case TX_SUCCESS:
            status = PHY_OK;
            break;
        case TX_NOT_AVAILABLE:
            status = PHY_BUSY;
            break;
        default:
            status = PHY_MUTEX_ERROR;
            break;
    }

    return status;
}


static phy_status_t phy_callback_give_mutex(void *context) {

    phy_status_t status = PHY_OK;

    /* Don't give the mutex if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return status;

    if (tx_mutex_put(&phy_mutex_handle) != TX_SUCCESS) status = PHY_MUTEX_ERROR;

    return status;
}

static phy_status_t phy_callback_event(phy_event_t event, void *context) {

    phy_status_t status = PHY_OK;

    switch (event) {
        case PHY_EVENT_LINK_UP:
        case PHY_EVENT_LINK_DOWN:

/* Notify the STP thread */
#if ENABLE_STP_THREAD == true

            /* Don't send notification if the kernel hasn't started */
            if (tx_thread_identify() == TX_NULL) return PHY_OK;

            tx_status_t tx_status = TX_SUCCESS;

            if (context == &hphy0) {
                tx_status = tx_event_flags_set(&stp_events_handle, STP_PORT0_LINK_STATE_CHANGE_EVENT, TX_OR);
            } else if (context == &hphy1) {
                tx_status = tx_event_flags_set(&stp_events_handle, STP_PORT1_LINK_STATE_CHANGE_EVENT, TX_OR);
            } else if (context == &hphy2) {
                tx_status = tx_event_flags_set(&stp_events_handle, STP_PORT2_LINK_STATE_CHANGE_EVENT, TX_OR);
            } else if (context == &hphy3) {
                tx_status = tx_event_flags_set(&stp_events_handle, STP_PORT3_LINK_STATE_CHANGE_EVENT, TX_OR);
            }
            /* Return the tx_status */
            if (tx_status != TX_SUCCESS) {
                status = PHY_ERROR;
            } else {
                status = PHY_OK;
            }
#endif

            break;

        default:
            break;
    }

    return status;
}

const phy_callbacks_t phy_callbacks_88q2112 = {
    .callback_read_reg_c22  = NULL,
    .callback_write_reg_c22 = NULL,
    .callback_read_reg_c45  = &phy_88q2112_callback_read_reg,
    .callback_write_reg_c45 = &phy_88q2112_callback_write_reg,
    .callback_get_time_ms   = &phy_callback_get_time_ms,
    .callback_delay_ms      = &phy_callback_delay_ms,
    .callback_delay_ns      = &phy_callback_delay_ns,
    .callback_take_mutex    = &phy_callback_take_mutex,
    .callback_give_mutex    = &phy_callback_give_mutex,
    .callback_event         = &phy_callback_event,
    .callback_write_log     = &ns_log_write,
};

const phy_callbacks_t phy_callbacks_lan8671 = {
    .callback_read_reg_c22  = &phy_lan8671_callback_read_reg,
    .callback_write_reg_c22 = &phy_lan8671_callback_write_reg,
    .callback_read_reg_c45  = NULL,
    .callback_write_reg_c45 = NULL,
    .callback_get_time_ms   = &phy_callback_get_time_ms,
    .callback_delay_ms      = &phy_callback_delay_ms,
    .callback_delay_ns      = &phy_callback_delay_ns,
    .callback_take_mutex    = &phy_callback_take_mutex,
    .callback_give_mutex    = &phy_callback_give_mutex,
    .callback_write_log     = &ns_log_write,
};

const phy_callbacks_t phy_callbacks_dp83867 = {
    .callback_read_reg_c22  = &phy_dp83867_callback_read_reg,
    .callback_write_reg_c22 = &phy_dp83867_callback_write_reg,
    .callback_read_reg_c45  = NULL,
    .callback_write_reg_c45 = NULL,
    .callback_get_time_ms   = &phy_callback_get_time_ms,
    .callback_delay_ms      = &phy_callback_delay_ms,
    .callback_delay_ns      = &phy_callback_delay_ns,
    .callback_take_mutex    = &phy_callback_take_mutex,
    .callback_give_mutex    = &phy_callback_give_mutex,
    .callback_write_log     = &ns_log_write,
};


void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {

    tx_status_t status       = TX_SUCCESS;
    uint32_t    flags_to_set = 0;

    /* Return if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return;

    switch (GPIO_Pin) {

        case (PHY0_INT_Pin):
            flags_to_set |= PHY_PHY0_EVENT;
            break;

        case (PHY1_INT_Pin):
            flags_to_set |= PHY_PHY1_EVENT;
            break;

        case (PHY2_INT_Pin):
            flags_to_set |= PHY_PHY2_EVENT;
            break;

        case (PHY3_INT_Pin):
            flags_to_set |= PHY_PHY3_EVENT;
            break;

        default:
            Error_Handler();
            break;
    }

    /* Set the flag */
    status = tx_event_flags_set(&phy_events_handle, flags_to_set, TX_OR);
    if (status != TX_SUCCESS) Error_Handler();
}
