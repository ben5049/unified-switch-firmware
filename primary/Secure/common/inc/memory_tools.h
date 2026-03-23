/*
 * memory_tools.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_MEMORY_TOOLS_H_
#define INC_MEMORY_TOOLS_H_

#include "stdint.h"
#include "stdbool.h"
#include "hal.h"
#include "logging.h"


#define CHECK_STATUS_MEM(status) CHECK_STATUS((status), MEM_OK, ERROR_MEM)


typedef enum {
    MEM_OK      = HAL_OK,
    MEM_ERROR   = HAL_ERROR,
    MEM_BUSY    = HAL_BUSY,
    MEM_TIMEOUT = HAL_TIMEOUT,
    MEM_NOT_IMPLEMENTED_ERROR,
    MEM_PARAMETER_ERROR,
    MEM_ALIGNMENT_ERROR,
    MEM_INVALID_ADDRESS_ERROR,
    MEM_POST_WRITE_CHECK_ERROR,
    MEM_SET_HASH_ERROR,
    MEM_HASH_INVALID_ERROR,
    MEM_PROGRAM_ERROR,
    MEM_UPDATE_NOT_STARTED_ERROR,
    MEM_SIGNATURE_ERROR,
} memory_status_t;


bool get_bank_swap(void);
void swap_banks(void);

void enable_backup_domain(void);

bool check_s_firmware(uint8_t bank);
bool check_ns_firmware(uint8_t bank);

memory_status_t copy_s_firmware_to_other_bank(bool bank_swap);
memory_status_t copy_ns_firmware_to_other_bank(bool bank_swap);
memory_status_t copy_ns_firmware(uint8_t from_bank, uint8_t to_bank);


#endif /* INC_MEMORY_TOOLS_H_ */
