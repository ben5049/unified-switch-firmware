/*
 * metadata.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_METADATA_H_
#define INC_METADATA_H_


#include "fram.h"
#include "integrity.h"
#include "prime256v1.h"
#include "nxd_dhcp_client.h"


#define METADATA_VERSION_MAJOR              0
#define METADATA_VERSION_MINOR              0
#define METADATA_VERSION_PATCH              0

#define METADATA_ENABLE_ROLLBACK_PROTECTION true

#define CHECK_STATUS_META(status)           CHECK_STATUS((status), META_OK, ERROR_META)


typedef enum {
    META_OK      = HAL_OK,
    META_ERROR   = HAL_ERROR,
    META_BUSY    = HAL_BUSY,
    META_TIMEOUT = HAL_TIMEOUT,
    META_NOT_IMPLEMENTED_ERROR,
    META_NOT_INITIALISED_ERROR,
    META_PARAMETER_ERROR,
    META_FRAM_ERROR,
    META_VERSION_ROLLBACK_ERROR,
    META_RNG_ERROR,
    META_MEMORY_ERROR,
    META_ENCRYPTION_ERROR,
    META_LOG_TOO_LONG_ERROR,
    META_LOCK_ERROR,
    META_ID_ERROR,
} metadata_status_t;

/* This struct stores the actual metadata data and is a mirror of the data stored in the FRAM. When this struct is changed the METADATA_VERSION numbers must be incremented. */
typedef struct __attribute__((__packed__)) {

    uint8_t metadata_version_major;
    uint8_t metadata_version_minor;
    uint8_t metadata_version_patch;

    /* Was the last shutdown a crash? */
    bool crashed;

    /* Firmware image data */
    bool    s_firmware_1_valid;
    uint8_t s_firmware_1_hash[SHA256_SIZE];
    bool    s_firmware_2_valid;
    uint8_t s_firmware_2_hash[SHA256_SIZE];
    bool    ns_firmware_1_valid;
    uint8_t ns_firmware_1_hash[SHA256_SIZE];
    bool    ns_firmware_2_valid;
    uint8_t ns_firmware_2_hash[SHA256_SIZE];

    /* Device ID computed from hash of 96-bit unique identifier */
    uint32_t device_id;

    /* DHCP Record for quickly restoring the IP address after reboot */
    NX_DHCP_CLIENT_RECORD dhcp_record;

} metadata_data_t;

/* This struct stores the actual metadata counters and is a mirror of the data stored in the FRAM. When this struct is changed the METADATA_VERSION numbers must be incremented. */
typedef struct __attribute__((__packed__)) {
    uint32_t crashes;
} metadata_counters_t;

typedef struct {
    fram_handle_t hfram;
    bool          first_boot;
    bool          initialised;
    uint32_t      device_id;
    bool          bank_swap;

    /* When a write is required set the following flags. The firmware should then periodically check and write if required */
    volatile bool new_metadata;
    volatile bool new_counters;

    __ALIGN_BEGIN metadata_data_t metadata     __ALIGN_END; /* Must be aligned to prevent non-maskable interrupts on non-byte accesses */
    __ALIGN_BEGIN metadata_counters_t counters __ALIGN_END; /* Must be aligned to prevent non-maskable interrupts on non-byte accesses */
} metadata_handle_t;


extern metadata_handle_t hmeta;


metadata_status_t META_Init(metadata_handle_t *meta, bool bank_swap);
metadata_status_t META_Reinit(metadata_handle_t *self, bool bank_swap);
metadata_status_t META_Configure(metadata_handle_t *meta);

metadata_status_t META_set_s_firmware_hash(metadata_handle_t *self, uint8_t bank, uint8_t *hash);
metadata_status_t META_check_s_firmware_hash(metadata_handle_t *self, uint8_t bank, uint8_t *hash, bool *valid);
metadata_status_t META_set_ns_firmware_hash(metadata_handle_t *self, uint8_t bank, uint8_t *hash);
metadata_status_t META_check_ns_firmware_hash(metadata_handle_t *self, uint8_t bank, uint8_t *hash, bool *valid);

metadata_status_t META_load_metadata(metadata_handle_t *self);
metadata_status_t META_dump_metadata(metadata_handle_t *self);
metadata_status_t META_load_counters(metadata_handle_t *self);
metadata_status_t META_dump_counters(metadata_handle_t *self);

metadata_status_t META_write_log(metadata_handle_t *self, uint16_t addr, const uint8_t *data, uint16_t size);
metadata_status_t META_read_log(metadata_handle_t *self, uint16_t addr, uint8_t *data, uint16_t size);


#endif /* INC_METADATA_H_ */
