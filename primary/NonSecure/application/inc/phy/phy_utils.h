/*
 * phy_utils.h
 *
 *  Created on: May 4, 2026
 *      Author: bens1
 */

#ifndef INC_PHY_UTILS_H_
#define INC_PHY_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdbool.h"

#include "phy.h"


#define PHY_CHECK(status)                        \
    do {                                         \
        if ((status) != PHY_OK) error_handler(); \
    } while (0)


#if HW_VERSION == 5
phy_status_t select_phy(phy_index_t phy_num, bool *switchover);
#endif


#ifdef __cplusplus
}
#endif

#endif /* INC_PHY_UTILS_H_ */
