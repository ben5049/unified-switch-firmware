/*
 * backup.c
 *
 *  Created on: Mar 24, 2026
 *      Author: bens1
 */

#include "stdbool.h"
#include "hal.h"
#include "ramcfg.h"

#include "backup.h"


volatile bool cold_boot = true;


// FIXME: In case of a brown-out or quick on -> off -> on it is possible for
//        the tamper registers to retain the magic word, but for the backup RAM
//        to lose power and therefore ECC data.
//
//        In the brown-out detector ISR, an addition bit should be set to warm
//        of a "brown" boot. This should cause an erase of the backup RAM next
//        boot, but the cold_boot flag should still be false.

hal_status_t enable_backup_domain() {

    hal_status_t  status = HAL_OK;
    __IO uint32_t tmpreg;

    /* Enable write access to backup domain */
    PWR_S->DBPCR |= PWR_DBPCR_DBP;

    /* Enable clock to backup domain */
    SET_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);

    /* Delay */
    tmpreg = READ_BIT(RCC_S->AHB1ENR, RCC_AHB1ENR_BKPRAMEN);
    UNUSED(tmpreg);

    /* Enable retention through sleep and power-off */
    PWR_S->BDCR |= PWR_BDCR_BREN;

    /* Enable voltage and temperature monitoring */
    PWR_S->BDCR |= PWR_BDCR_MONEN;

    /* Check if this is a cold boot */
    if (TAMP->BACKUP_MAGIC_REG != BACKUP_MAGIC_WORD) {

        cold_boot = true;

        /* Erase the backup RAM to prevent ECC errors */
        status = HAL_RAMCFG_Erase(&hramcfg_BKPRAM);
        if (status != HAL_OK) return status;

        /* Set the magic word so next boot we know BKPRAM ECC is valid */
        TAMP->BACKUP_MAGIC_REG = BACKUP_MAGIC_WORD;
    }

    else {
        cold_boot = false;
    }

    return status;
}
