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
