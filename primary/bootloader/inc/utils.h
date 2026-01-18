/*
 * utils.h
 *
 *  Created on: Aug 19, 2025
 *      Author: bens1
 */

#ifndef INC_UTILS_H_
#define INC_UTILS_H_


#include "hal.h"


#define CURRENT_FLASH_BANK(swap) ((swap) ? (FLASH_BANK_2) : (FLASH_BANK_1))
#define OTHER_FLASH_BANK(swap)   ((swap) ? (FLASH_BANK_1) : (FLASH_BANK_2))


#endif /* INC_UTILS_H_ */
