/*
 * bootloader_callbacks.c
 *
 *  Created on: Mar 23, 2026
 *      Author: bens1
 */

#include "hal.h"
#include "stdint.h"
#include "assert.h"

#include "main.h"
#include "bootloader_callbacks.h"
#include "memory_tools.h"


static bootloader_status_t bootloader_spi_transmit(const uint32_t *data, uint16_t size, uint32_t timeout, void *context){
    hal_status_t status = HAL_OK;
    status = HAL_SPI_Transmit(&hspi1, data, size, timeout);
    return (status == HAL_OK) ? BOOTLOADER_OK : BOOTLOADER_IO_ERROR;
}

static bootloader_status_t bootloader_spi_receive(uint32_t *data, uint16_t size, uint32_t timeout, void *context){
    hal_status_t status = HAL_OK;
    status = HAL_SPI_Receive(&hspi1, data, size, timeout);
    return (status == HAL_OK) ? BOOTLOADER_OK : BOOTLOADER_IO_ERROR;
}

static bootloader_status_t bootloader_uart_transmit(const uint32_t *data, uint16_t size, uint32_t timeout, void *context){
    return BOOTLOADER_NOT_IMPLEMENTED_ERROR; // TODO: write this
}

static bootloader_status_t bootloader_uart_receive(uint32_t *data, uint16_t size, uint32_t timeout, void *context){
    return BOOTLOADER_NOT_IMPLEMENTED_ERROR; // TODO: write this
}

static uint32_t bootloader_get_time_ms(void *context){
    return HAL_GetTick();
}

static void bootloader_delay_ms(uint32_t ms, void *context){
    HAL_Delay(ms);
}

static void bootloader_write_pin(bootloader_pin_t pin_name, bootloader_pinstate_t state){

    gpio_t *port = NULL;
    uint16_t pin;

    switch (pin_name) {

        case BL_PIN_STAT:
            port = STAT_GPIO_Port;
            pin  = STAT_Pin;
            break;

        case BL_PIN_PWR_EN:
            port = PWR_EN_GPIO_Port;
            pin  = PWR_EN_Pin;
            break;

            case BL_PIN_FRAM_CS:
            port = FRAM_CS_GPIO_Port;
            pin  = FRAM_CS_Pin;
            break;

        case BL_PIN_FRAM_HOLD:
            port = FRAM_HOLD_GPIO_Port;
            pin  = FRAM_HOLD_Pin;
            break;

        case BL_PIN_FRAM_WP:
            port = FRAM_WP_GPIO_Port;
            pin  = FRAM_WP_Pin;
            break;
    }
    assert(port != NULL);

    if (state == BL_PIN_RESET){
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    }
}

/* Hardware erase the non-secure applications's RAM */
static bootloader_status_t bootloader_erase_app_ram(){
    status = HAL_RAMCFG_Erase(&hramcfg_SRAM3);
    return (status == HAL_OK) ? BOOTLOADER_OK : BOOTLOADER_ERROR;
}

/* Hardware erase the log buffer RAM */
static bootloader_status_t bootloader_erase_app_ram(){
    status = HAL_RAMCFG_Erase(&hramcfg_SRAM1);
    return (status == HAL_OK) ? BOOTLOADER_OK : BOOTLOADER_ERROR;
}


const bootloader_callbacks_t sja1105_callbacks = {
    .callback_spi_transmit         = &bootloader_spi_transmit,
    .callback_spi_receive          = &bootloader_spi_receive,
    .callback_uart_transmit        = &bootloader_uart_transmit,
    .callback_uart_receive         = &bootloader_uart_receive,
    .callback_get_time_ms          = &bootloader_get_time_ms,
    .callback_delay_ms             = &bootloader_delay_ms,
    .callback_write_pin            = &bootloader_write_pin,
    .callback_enable_backup_domain = &enable_backup_domain,
    .callback_enable_systick       = &HAL_ResumeTick,
    .callback_disable_systick      = &HAL_SuspendTick,
    .callback_erase_app_ram        = &bootloader_erase_app_ram,
    .callback_erase_log_ram        = &bootloader_erase_log_ram,
};
