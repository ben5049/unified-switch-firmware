/*
 * stm32_uidhash.h
 *
 *  Created on: Mar 15, 2024
 *      Author: https://pcbartists.com/firmware/stm32-firmware/generating-32-bit-stm32-unique-id/
 */

#ifndef INC_STM32_UIDHASH_H_
#define INC_STM32_UIDHASH_H_

#include "stdint.h"

uint32_t Hash32Len5to12(const uint8_t *s, size_t len);

#endif /* INC_STM32_UIDHASH_H_ */
