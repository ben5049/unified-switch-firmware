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
#define PHY_PHY0_EVENT ((ULONG) 1 << 0)
#define PHY_PHY1_EVENT ((ULONG) 1 << 1)
#define PHY_PHY2_EVENT ((ULONG) 1 << 2)
#define PHY_PHY3_EVENT ((ULONG) 1 << 3)
#if HW_VERSION == 5
#define PHY_PHY4_EVENT ((ULONG) 1 << 4)
#define PHY_PHY5_EVENT ((ULONG) 1 << 5)
#define PHY_PHY6_EVENT ((ULONG) 1 << 6)
#endif
#define PHY_ALL_EVENTS ((ULONG) 0xffffffff)


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
