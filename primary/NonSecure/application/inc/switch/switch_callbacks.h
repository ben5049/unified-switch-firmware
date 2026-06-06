/*
 * switch_callbacks.h
 *
 *  Created on: Aug 5, 2025
 *      Author: bens1
 */

#ifndef INC_SWITCH_CALLBACKS_H_
#define INC_SWITCH_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "switch.h"


#define SWCH_SPI   hspi2
#define SWCH_CRC   hcrc

#define SWCH_POOL0 (1 << 0)
#if HW_VERSION == 5
#define SWCH_POOL1 (1 << 1)
#endif


typedef enum {
    SWITCH_EVENT_MAINTENANCE = 1UL << 0,
    SWITCH_EVENT_PUBLISH     = 1UL << 1,
} switch_event_t;


/* Imported variables */
extern SPI_HandleTypeDef SWCH_SPI;
extern CRC_HandleTypeDef SWCH_CRC;


/* Exported variables */
extern TX_MUTEX                  switch_mutex_handle;
extern const sja1105_callbacks_t sja1105_callbacks;


sja1105_status_t switch_byte_pool_init(uint8_t pool);

void switch_maintenance_timer_callback(ULONG id);
void switch_publish_timer_callback(ULONG id);


#ifdef __cplusplus
}
#endif

#endif /* INC_SWITCH_CALLBACKS_H_ */
