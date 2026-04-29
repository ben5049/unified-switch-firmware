/*
 * backup.c
 *
 *  Created on: Mar 24, 2026
 *      Author: bens1
 */

#include "hal.h"
#include "ramcfg.h"

#include "backup.h"


hal_status_t enable_backup_domain() {

    hal_status_t status = HAL_OK;

    /* Enable write access to backup domain */
    PWR_S->DBPCR |= PWR_DBPCR_DBP;

    /* Enable clock to backup domain */
    __IO uint32_t tmpreg;
    SET_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);

    /* Delay after an RCC peripheral clock enabling */
    tmpreg = READ_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);
    UNUSED(tmpreg);

    /* Enable retention through sleep and power-off */
    PWR_S->BDCR |= PWR_BDCR_BREN;

    /* Enable voltage and temperature monitoring. TODO: Use this? */
    // PWR_S->BDCR  |= PWR_BDCR_MONEN;

    /* Check if this is a cold boot */
    if (TAMP->BACKUP_MAGIC_REG != BACKUP_MAGIC_WORD) {

        /* Erase the backup RAM to prevent ECC errors */
        status = HAL_RAMCFG_Erase(&hramcfg_BKPRAM);
        if (status != HAL_OK) return status;

        /* Set the magic word so next boot we know BKPRAM ECC is valid */
        TAMP->BACKUP_MAGIC_REG = BACKUP_MAGIC_WORD;
    }

    return status;
}
