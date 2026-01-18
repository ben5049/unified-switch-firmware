/*
 * integrity.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_INTEGRITY_H_
#define INC_INTEGRITY_H_


#include "stdint.h"
#include "stdbool.h"
#include "hal.h"
#include "hash.h"
#include "utils.h"

#define SHA256_SIZE                    (32)  /* Size in bytes */
#define INTEGRITY_TIMEOUT_MS           (500) /* ms */


#define CHECK_STATUS_INTEGRITY(status) CHECK_STATUS((status), INTEGRITY_OK, ERROR_INTEGRITY)


typedef enum {
    INTEGRITY_OK      = HAL_OK,
    INTEGRITY_ERROR   = HAL_ERROR,
    INTEGRITY_BUSY    = HAL_BUSY,
    INTEGRITY_TIMEOUT = HAL_TIMEOUT,
    INTEGRITY_NOT_IMPLEMENTED_ERROR,
    INTEGRITY_PARAMETER_ERROR,
    INTEGRITY_HASHING_ERROR,
} integrity_status_t;

typedef enum {
    INTEGRITY_HASH_NOT_COMPUTED,
    INTEGRITY_HASH_IN_PROGRESS,
    INTEGRITY_HASH_COMPLETE,
    INTEGRITY_HASH_ERROR,
} integrity_state_t;


integrity_status_t INTEGRITY_Init(bool bank_swap, uint8_t *current_bank_secure_digest, uint8_t *other_bank_secure_digest, uint8_t *current_bank_non_secure_digest, uint8_t *other_bank_non_secure_digest);

/* Hash functions */
integrity_status_t INTEGRITY_compute_s_firmware_hash(uint8_t bank);
integrity_status_t INTEGRITY_compute_ns_firmware_hash(uint8_t bank);
uint8_t           *INTEGRITY_get_s_firmware_hash(uint8_t bank);
uint8_t           *INTEGRITY_get_ns_firmware_hash(uint8_t bank);
bool               INTEGRITY_get_hash_in_progress(void);

/* TODO: */
bool INTEGRITY_check_firmware_signature(uint8_t *hash, uint8_t *signature_r, uint8_t *signature_s);


#endif /* INC_INTEGRITY_H_ */
