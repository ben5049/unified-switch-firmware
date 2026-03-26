/*
 * backup.c
 *
 *  Created on: Mar 24, 2026
 *      Author: bens1
 */

#include "hal.h"
#include "backup.h"


void enable_backup_domain() {

    /* Enable write access to backup domain */
    PWR_S->DBPCR |= PWR_DBPCR_DBP; /* Disable write protection */

    /* Enable clock to backup domain (updated __HAL_RCC_BKPRAM_CLK_ENABLE() for secure world) */
    __IO uint32_t tmpreg;
    SET_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);
    /* Delay after an RCC peripheral clock enabling */
    tmpreg = READ_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);
    UNUSED(tmpreg);

    /* Enable retention through sleep and power-off */
    PWR_S->BDCR |= PWR_BDCR_BREN;

    /* Enable voltage and temperature monitoring. TODO: Use this? */
    // PWR_S->BDCR  |= PWR_BDCR_MONEN;
}
