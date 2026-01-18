/*
 * hal.h
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#ifndef INC_HAL_H_
#define INC_HAL_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stm32h5xx_hal.h"
#include "stm32h573xx.h"

#include "stm32h5xx_hal_eth.h" /* Being explicit */


typedef HAL_StatusTypeDef hal_status_t;


#ifdef __cplusplus
}
#endif

#endif /* INC_HAL_H_ */
