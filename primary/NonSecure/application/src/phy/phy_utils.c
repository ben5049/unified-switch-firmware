/*
 * phy_utils.c
 *
 *  Created on: May 4, 2026
 *      Author: bens1
 *
 */

#include "hal.h"

#include "app.h"
#include "phy_utils.h"
#include "utils.h"


/* 88Q2112 PHY MDIO lines are selected by an external mux */
#if HW_VERSION == 5
phy_status_t select_phy(phy_index_t phy_num, bool *switchover) {

    phy_status_t   status       = PHY_OK;
    static uint8_t prev_phy_num = NUM_PHYS;

    /* Mux already has the correct PHY selected */
    if (phy_num == prev_phy_num) {
        if (switchover != NULL) *switchover = false;
        return status;
    } else {
        if (switchover != NULL) *switchover = true;
    }

    /* Mux selection */
    switch (phy_num) {

        /* Channel Y5 */
        case PHY1_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_SET);
            break;

        /* Channel Y4 */
        case PHY2_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_SET);
            break;

        /* Channel Y2 */
        case PHY3_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_RESET);
            break;

        /* Channel Y1 */
        case PHY4_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_RESET);
            break;

        /* Channel Y3 */
        case PHY5_88Q2112:
            HAL_GPIO_WritePin(SMI_SEL1_GPIO_Port, SMI_SEL1_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL2_GPIO_Port, SMI_SEL2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(SMI_SEL3_GPIO_Port, SMI_SEL3_Pin, GPIO_PIN_RESET);
            break;

        default:
            status = PHY_PARAMETER_ERROR; /* Only 88Q2112 PHYs are muxed */
            break;
    }

    /* Delay while mux switches over and pullup resistors take effect */
    delay_ns(500);
    prev_phy_num = phy_num;

    return status;
}
#endif
