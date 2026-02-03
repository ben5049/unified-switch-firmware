/*
 * nx_link_driver.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 *
 *  This file is the equivalent to nx_stm32_phy_driver.c
 *
 */

#include <stdbool.h>
#include "tx_api.h"
#include "nx_stm32_eth_config.h"
#include "nx_stm32_phy_driver.h"

#include "nx_app.h"
#include "switch_thread.h"
#include "phy_thread.h"
#include "config.h"


int32_t nx_eth_phy_init(void) {

    int32_t ret = ETH_PHY_STATUS_OK;

    /* Check the switch initialised flag */
    if (!hsw0.initialised) ret = ETH_PHY_STATUS_ERROR;

    return ret;
}


int32_t nx_eth_phy_get_link_state(void) {

    int32_t linkstate           = ETH_PHY_STATUS_LINK_ERROR;
    bool    phy_link_up         = false;
    bool    external_connection = false;

    /* If SJA1105 isn't initialised or none of the PHYs have links then return link down */
    phy_link_up = hphy0.linkup || hphy1.linkup || hphy2.linkup || hphy3.linkup
#if HW_VERSION == 5
                  || hphy4.linkup || hphy5.linkup || hphy6.linkup
#endif
        ;

    external_connection = hsw0.initialised
#if HW_VERSION == 5
                          && hsw1.initialised
#endif
                          && (phy_link_up || !PHY_LINK_REQUIRED_FOR_NX_LINK);

    if (!external_connection) {
        linkstate = ETH_PHY_STATUS_LINK_DOWN;
        return linkstate;
    }

    switch (hsw0.config->ports[SW0_PORT_HOST].speed) {
        case SJA1105_SPEED_10M:
            linkstate = ETH_PHY_STATUS_10MBITS_FULLDUPLEX;
            break;

        case SJA1105_SPEED_100M:
            linkstate = ETH_PHY_STATUS_100MBITS_FULLDUPLEX;
            break;

#ifdef ETH_PHY_1000MBITS_SUPPORTED
        case SJA1105_SPEED_1G:
            linkstate = ETH_PHY_STATUS_1000MBITS_FULLDUPLEX;
            break;
#endif

        default:
            linkstate = ETH_PHY_STATUS_LINK_ERROR;
            break;
    }

    return linkstate;
}
