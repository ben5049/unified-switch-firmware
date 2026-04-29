/*
 * secure_nsc.c
 *
 *  Created on: Oct 5, 2025
 *      Author: bens1
 */

#include "string.h"

#include "app.h"
#include "bootloader.h"
#include "secure_nsc.h"
#include "error.h"
#include "logging.h"
#include "utils.h"
#include "bootloader_config.h"


/* Call this function from a thread in ThreadX at 1Hz to do background work in the secure world */
CMSE_NS_ENTRY void s_background_task(void) {

    enable_tick();

    bootloader_status_t status = BL_OK;

    /* Sync the metadata if required */
    status = bootloader_sync_metadata(false);
    if (status != BL_OK) error_handler(status);

    // TODO: Scrub the flash (and RAM for ECC errors)

    disable_tick();
}


CMSE_NS_ENTRY int s_write_user_storage(uint16_t addr, const uint8_t *data, uint16_t size) {

    enable_tick();

    bootloader_status_t status = BL_OK;

    status = bootloader_write_user_storage(addr, data, size);

    disable_tick();

    return (status == BL_OK) ? size : -1;
}


CMSE_NS_ENTRY int s_read_user_storage(uint16_t addr, uint8_t *data, uint16_t size) {

    enable_tick();

    bootloader_status_t status = BL_OK;

    status = bootloader_read_user_storage(addr, data, size);

    disable_tick();

    return (status == BL_OK) ? size : -1;
}


/* Called by the application when it reaches an unrecoverable state */
CMSE_NS_ENTRY void s_error_handler() {

    enable_tick();

    app_crashed();

    disable_tick();
}


CMSE_NS_ENTRY uint32_t s_random_u32() {

    enable_tick();

    bootloader_status_t status = BL_OK;
    uint32_t            number;

    status = bootloader_get_random_u32(&number);
    if (status != BL_OK) error_handler(status);

    disable_tick();

    return number;
}


extern int _write(int file, char *ptr, int len);

CMSE_NS_ENTRY int s_write(int file, char *ptr, int len) {

    enable_tick();

    int ret = 0;

#if UART_LOGGING_ENABLE
    _write(file, ptr, len);
#endif

    disable_tick();

    return ret;
}


CMSE_NS_ENTRY bool s_uart_logging_enabled(void) {
    return UART_LOGGING_ENABLE;
}
