/*
 * phy_callbacks.h
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 */

#ifndef INC_PHY_CALLBACKS_H_
#define INC_PHY_CALLBACKS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "88q211x.h"
#include "lan867x.h"
#if HW_VERSION == 5
#include "dp83867.h"
#endif


/* Note: these must be in order */
typedef enum {
    PHY_EVENT_IRQ0 = 1UL << 0,
    PHY_EVENT_IRQ1 = 1UL << 1,
    PHY_EVENT_IRQ2 = 1UL << 2,
    PHY_EVENT_IRQ3 = 1UL << 3,
#if HW_VERSION == 5
    PHY_EVENT_IRQ4 = 1UL << 4,
    PHY_EVENT_IRQ5 = 1UL << 5,
    PHY_EVENT_IRQ6 = 1UL << 6,
#endif
    PHY_EVENT_UPDATE_STATE = 1UL << 16,
    PHY_EVENT_READ_TEMP    = 1UL << 17,
    PHY_EVENT_ALL          = 0xffffffffUL
} phy_thread_event_t;


extern const phy_callbacks_t phy_callbacks_88q2112;
extern const phy_callbacks_t phy_callbacks_lan8671;
#if HW_VERSION == 5
extern const phy_callbacks_t phy_callbacks_dp83867;
#endif

extern TX_MUTEX phy_mutex_handle;

extern TX_EVENT_FLAGS_GROUP phy_events_handle;


#ifdef __cplusplus
}
#endif

#endif /* INC_PHY_CALLBACKS_H_ */
