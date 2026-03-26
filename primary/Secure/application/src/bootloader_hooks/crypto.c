/*
 * crypto.c
 *
 *  Created on: Mar 26, 2026
 *      Author: bens1
 */

#include "hal.h"
#include "rng.h"
#include "aes.h"
#include "hash.h"
#include "pka.h"

#include "bootloader.h"


uint32_t bootloader_get_uid(uint8_t word) {
    switch (word) {
        case 0:
            return HAL_GetUIDw0();
        case 1:
            return HAL_GetUIDw1();
        case 2:
            return HAL_GetUIDw2();
        default:
            return 0;
    }
}


bootloader_status_t bootloader_get_random_u32(uint32_t *random_number) {
    hal_status_t status = HAL_RNG_GenerateRandomNumber(&hrng, random_number);
    return (status == HAL_OK) ? BL_OK : BL_RNG_ERROR;
}


bootloader_status_t bootloader_hash(uint32_t addr, uint32_t size, uint8_t hash[HASH_SIZE], uint32_t timeout) {
    hal_status_t status = HAL_HASH_Start(&hhash, (uint8_t *) addr, size, hash, timeout);
    return (status == HAL_OK) ? BL_OK : BL_HASH_ERROR;
}


bootloader_status_t bootloader_encrypt(uint32_t *in, uint16_t size, uint32_t *out, uint32_t timeout) {
    hal_status_t status = HAL_CRYP_Encrypt(&hcryp, in, size, out, timeout);
    return (status == HAL_OK) ? BL_OK : BL_ENCRYPTION_ERROR;
}


bootloader_status_t bootloader_decrypt(uint32_t *in, uint16_t size, uint32_t *out, uint32_t timeout) {
    hal_status_t status = HAL_CRYP_Decrypt(&hcryp, in, size, out, timeout);
    return (status == HAL_OK) ? BL_OK : BL_ENCRYPTION_ERROR;
}
