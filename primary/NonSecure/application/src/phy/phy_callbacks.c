/*
 * phy_callbacks.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#include "stdint.h"
#include "stdbool.h"

#include "hal.h"
#include "tx_api.h"

#include "app.h"
#include "tx_app.h"
#include "88q211x.h"
#include "lan867x.h"
#include "phy_callbacks.h"
#include "phy_thread.h"
#include "phy_utils.h"
#include "stp_thread.h"
#include "utils.h"
#include "phy_platform.h"


TX_MUTEX             phy_mutex_handle;
TX_EVENT_FLAGS_GROUP phy_events_handle;


static phy_status_t phy_88q2112_callback_read_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    phy_status_t status   = PHY_OK;
    bool         preamble = false;

    /* Select the PHY */
#if HW_VERSION == 5
    status = select_phy(((phy_info_t *) context)->index, &preamble);
    PHY_CHECK_RET(status);
#endif

    /* 88Q2112 only needs 1 preamble bit */
    /* Set the clock frequency to 9.62MHz (PHY supports up to 12.5MHz) */
    /* If switched over send the preamble for the first transaction */
    status = phy_read_reg_c45(phy_addr, mmd_addr, reg_addr, data, timeout, !preamble, ETH_MACMDIOAR_CR_DIV26);

    return status;
}

static phy_status_t phy_lan8671_callback_read_reg(uint8_t phy_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 2.45MHz (PHY supports up to 4MHz) */
    return phy_read_reg_c22(phy_addr, reg_addr, data, timeout, false, ETH_MACMDIOAR_CR_DIV102);
}

#if HW_VERSION == 5
static phy_status_t phy_dp83867_callback_read_reg(uint8_t phy_addr, uint16_t reg_addr, uint16_t *data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 15.6MHz (PHY supports up to 25MHz) */
    return phy_read_reg_c22(phy_addr, reg_addr, data, timeout, false, ETH_MACMDIOAR_CR_DIV16);
}
#endif


static phy_status_t phy_88q2112_callback_write_reg(uint8_t phy_addr, uint8_t mmd_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    phy_status_t status   = PHY_OK;
    bool         preamble = false;

    /* Select the PHY */
#if HW_VERSION == 5
    status = select_phy(((phy_info_t *) context)->index, &preamble);
    PHY_CHECK_RET(status);
#endif

    /* 88Q2112 only needs 1 preamble bit */
    /* Set the clock frequency to 9.62MHz (PHY supports up to 12.5MHz) */
    /* If switched over send the preamble for the first transaction */
    status = phy_write_reg_c45(phy_addr, mmd_addr, reg_addr, data, timeout, !preamble, ETH_MACMDIOAR_CR_DIV26);

    return status;
}

static phy_status_t phy_lan8671_callback_write_reg(uint8_t phy_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 2.45MHz (PHY supports up to 4MHz) */
    return phy_write_reg_c22(phy_addr, reg_addr, data, timeout, false, ETH_MACMDIOAR_CR_DIV102);
}

#if HW_VERSION == 5
static phy_status_t phy_dp83867_callback_write_reg(uint8_t phy_addr, uint16_t reg_addr, uint16_t data, uint32_t timeout, void *context) {

    /* Set the clock frequency to 15.6MHz (PHY supports up to 25MHz) */
    return phy_write_reg_c22(phy_addr, reg_addr, data, timeout, false, ETH_MACMDIOAR_CR_DIV16);
}
#endif


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
            LOG_INFO("PHY%d Event: link up", ((phy_info_t *) context)->index);
            break;

        case PHY_EVENT_LINK_DOWN:
            LOG_INFO("PHY%d Event: link down", ((phy_info_t *) context)->index);
            break;

/* Notify the STP thread */
#if ENABLE_STP_THREAD == true

            /* Don't send notification if the kernel hasn't started */
            if (tx_thread_identify() == TX_NULL) return PHY_OK;

            tx_status_t tx_status = TX_SUCCESS;

#error "TODO: context is the PHY number not handle"
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

            break;
#endif

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
    .callback_write_log     = &log_info,
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
    .callback_event         = &phy_callback_event,
    .callback_write_log     = &log_info,
};

#if HW_VERSION == 5
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
    .callback_event         = &phy_callback_event,
    .callback_write_log     = &log_info,
};
#endif


void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin) {

    tx_status_t status       = TX_SUCCESS;
    uint32_t    flags_to_set = 0;

    /* Return if the kernel hasn't started */
    if (tx_thread_identify() == TX_NULL) return;

    switch (GPIO_Pin) {

#if NUM_PHYS > 0
        case (PHY0_INT_Pin):
            flags_to_set |= PHY_PHY0_EVENT;
            break;
#endif
#if NUM_PHYS > 1
        case (PHY1_INT_Pin):
            flags_to_set |= PHY_PHY1_EVENT;
            break;
#endif
#if NUM_PHYS > 2
        case (PHY2_INT_Pin):
            flags_to_set |= PHY_PHY2_EVENT;
            break;
#endif
#if NUM_PHYS > 3
        case (PHY3_INT_Pin):
            flags_to_set |= PHY_PHY3_EVENT;
            break;
#endif
#if NUM_PHYS > 4
        case (PHY4_INT_Pin):
            flags_to_set |= PHY_PHY4_EVENT;
            break;
#endif
#if NUM_PHYS > 5
        case (PHY5_INT_Pin):
            flags_to_set |= PHY_PHY5_EVENT;
            break;
#endif
#if NUM_PHYS > 6
        case (PHY6_INT_Pin):
            flags_to_set |= PHY_PHY6_EVENT;
            break;
#endif
#if NUM_PHYS > 7
#error "Unsupported number of PHYs"
#endif

        default:
            error_handler();
            break;
    }

    /* Set the flag */
    status = tx_event_flags_set(&phy_events_handle, flags_to_set, TX_OR);
    if (status != TX_SUCCESS) error_handler();
}
