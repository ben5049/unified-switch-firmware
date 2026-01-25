/*
 * sja1105_callbacks.h
 *
 *  Created on: Aug 5, 2025
 *      Author: bens1
 */

#ifndef INC_SWITCH_SJA1105_CALLBACKS_H_
#define INC_SWITCH_SJA1105_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "sja1105.h"


#define SWCH_SPI   hspi2
#define SWCH_CRC   hcrc

#define SWCH_POOL0 (1 << 0)
#define SWCH_POOL1 (1 << 1)


/* Imported variables */
extern SPI_HandleTypeDef SWCH_SPI;
extern CRC_HandleTypeDef SWCH_CRC;


/* Exported variables */
extern TX_MUTEX                  sja1105_mutex_handle;
extern const sja1105_callbacks_t sja1105_callbacks;


sja1105_status_t switch_byte_pool_init(uint8_t pool);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_SJA1105_CALLBACKS_H_ */
