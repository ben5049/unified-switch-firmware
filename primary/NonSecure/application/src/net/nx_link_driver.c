/*
 * nx_link_driver.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 *
 *  This file is the equivalent to nx_stm32_phy_driver.c
 *
 */

#include "nx_stm32_eth_config.h"
#include "nx_stm32_phy_driver.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "switch.h"
#include "phy.h"
#include "validation.h"


int32_t nx_eth_phy_init(void) {

    int32_t ret = ETH_PHY_STATUS_OK;

    /* Check the switch initialised flag */
    if (!switch_handles[SWITCH0].initialised) ret = ETH_PHY_STATUS_ERROR;

    return ret;
}


int32_t nx_eth_phy_get_link_state(void) {

    int32_t          linkstate           = ETH_PHY_STATUS_LINK_ERROR;
    bool             phy_link_up         = false;
    bool             external_connection = true;
    sja1105_status_t switch_status       = SJA1105_OK;
    uint16_t         speed;

    /* If SJA1105 isn't initialised or none of the PHYs have links then return link down
     * TODO: v4 hphy3 and v5 hphy6 use PLCA and always count as having their link up. need to find better linkup metric. e.g. frames received */
    phy_link_up = hphy0.linkup || hphy1.linkup || hphy2.linkup || hphy3.linkup
#if HW_VERSION == 5
                  || hphy4.linkup || hphy5.linkup || hphy6.linkup
#endif
        ;

    /* AND reduction of switch statuses and other requirements */
    for (switch_index_t i = 0; i < NUM_SWITCHES; i++) {
        external_connection = external_connection && switch_handles[i].initialised;
    }
    external_connection = external_connection && (phy_link_up || !PHY_LINK_REQUIRED_FOR_NX_LINK);

    if (!external_connection) {
        linkstate = ETH_PHY_STATUS_LINK_DOWN;
        return linkstate;
    }

    switch_status = switch_get_speed(PORT_HOST, &speed);
    SWITCH_CHECK(switch_status);
    switch (speed) {

        case 10:
            linkstate = ETH_PHY_STATUS_10MBITS_FULLDUPLEX;
            break;

        case 100:
            linkstate = ETH_PHY_STATUS_100MBITS_FULLDUPLEX;
            break;

#ifdef ETH_PHY_1000MBITS_SUPPORTED
        case 1000:
            linkstate = ETH_PHY_STATUS_1000MBITS_FULLDUPLEX;
            break;
#endif

        default:
            linkstate = ETH_PHY_STATUS_LINK_ERROR;
            VAL_TERMINATE();
            break;
    }

    return linkstate;
}
