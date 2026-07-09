/*
 * phy_utils.c
 *
 *  Created on: May 4, 2026
 *      Author: bens1
 *
 */

#include "hal.h"

#include "app.h"
#include "phy.h"
#include "utils.h"


#define NUM_LATENCY_CORRECTIONS (3)
#define UNR                     (UINT16_MAX) /* Unreachable */


/* clang-format off */
static const uint16_t ingress_latency_corrections[NUM_PHYS][NUM_LATENCY_CORRECTIONS] = {

    /* Speed (mbps)
     * 10   100  1000 */
#if HW_VERSION == 4
    { UNR,  750,    0}, /* PHY0: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY1: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY2: 1000BASE-T1 */
    {   0,  UNR,  UNR}, /* PHY3: 10BASE-T1S  */
#elif HW_VERSION == 5
    { 275,  275,  275}, /* PHY0: 1000BASE-T  */
    { UNR,  750,    0}, /* PHY1: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY2: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY3: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY4: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY5: 1000BASE-T1 */
    {   0,  UNR,  UNR}, /* PHY6: 10BASE-T1S  */
#endif
};

static const uint16_t egress_latency_corrections[NUM_PHYS][NUM_LATENCY_CORRECTIONS] = {

    /* Speed (mbps)
     * 10   100  1000 */
#if HW_VERSION == 4
    { UNR,  750,    0}, /* PHY0: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY1: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY2: 1000BASE-T1 */
    {   0,  UNR,  UNR}, /* PHY3: 10BASE-T1S  */
#elif HW_VERSION == 5
    { 275,  275,  275}, /* PHY0: 1000BASE-T  */
    { UNR,  750,    0}, /* PHY1: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY2: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY3: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY4: 1000BASE-T1 */
    { UNR,  750,    0}, /* PHY5: 1000BASE-T1 */
    {   0,  UNR,  UNR}, /* PHY6: 10BASE-T1S  */
#endif
};
/* clang-format on */


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


phy_status_t update_phy_latencies(phy_index_t phy) {

    assert(phy < NUM_PHYS);

    phy_status_t       status = PHY_OK;
    phy_handle_base_t *hphy   = phy_handles[phy];
    uint16_t           ingress_latency, egress_latency;
    uint16_t           ingress_correction, egress_correction;

    /* Get raw latencies */
    status = PHY_GetIngressLatency(hphy, &ingress_latency);
    if (status != PHY_OK) return status;
    status = PHY_GetEgressLatency(hphy, &egress_latency);
    if (status != PHY_OK) return status;

    /* Get corrections */
    assert(hphy->speed - PHY_SPEED_10M < NUM_LATENCY_CORRECTIONS);
    ingress_correction = ingress_latency_corrections[phy][hphy->speed - PHY_SPEED_10M];
    if (ingress_correction == UNR) status = PHY_INVALID_SPEED_ERROR;
    if (status != PHY_OK) return status;
    egress_correction = egress_latency_corrections[phy][hphy->speed - PHY_SPEED_10M];
    if (egress_correction == UNR) status = PHY_INVALID_SPEED_ERROR;
    if (status != PHY_OK) return status;

    /* Apply corrections */
    phy_ingress_latencies[phy] = ingress_latency + ingress_correction;
    phy_egress_latencies[phy]  = egress_latency + egress_correction;

    return status;
}
