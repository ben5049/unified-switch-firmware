/*
 * io.c
 *
 *  Created on: Mar 26, 2026
 *      Author: bens1
 */

#include "hal.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

#include "error.h"
#include "bootloader.h"


bootloader_status_t bootloader_spi_transmit(const uint8_t *data, uint16_t size, uint32_t timeout) {
    hal_status_t status = HAL_SPI_Transmit(&hspi1, data, size, timeout);
    return (status == HAL_OK) ? BL_OK : BL_IO_ERROR;
}


bootloader_status_t bootloader_spi_receive(uint8_t *data, uint16_t size, uint32_t timeout) {
    hal_status_t status = HAL_SPI_Receive(&hspi1, data, size, timeout);
    return (status == HAL_OK) ? BL_OK : BL_IO_ERROR;
}


bootloader_status_t bootloader_uart_transmit(const uint8_t *data, uint16_t size, uint32_t timeout) {
    return BL_NOT_IMPLEMENTED_ERROR; // TODO: write this
}


bootloader_status_t bootloader_uart_receive(uint8_t *data, uint16_t size, uint32_t timeout) {
    return BL_NOT_IMPLEMENTED_ERROR; // TODO: write this
}


void bootloader_write_pin(bootloader_pin_t pin_name, bootloader_pinstate_t state) {

    gpio_t  *port = NULL;
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
    if (port != NULL) error_handler(BL_NOT_IMPLEMENTED_ERROR);

    if (state == BL_PIN_RESET) {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    }
}