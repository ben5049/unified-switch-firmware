/*
 * backup.h
 *
 *  Created on: Mar 24, 2026
 *      Author: bens1
 */

#ifndef INC_BACKUP_H_
#define INC_BACKUP_H_


/* One of the 32 backup registers is used to store a magic word telling
 * us if the BKPRAM is valid.
 *
 * This can't be stored in the BKPRAM directly since it has ECC and
 * reading ECC memory from a cold boot results in an NMI */

#define BACKUP_MAGIC_REG  BKP0R
#define BACKUP_MAGIC_WORD (0x738afea1)


hal_status_t enable_backup_domain(void);


#endif /* INC_BACKUP_H_ */
