/*
 * app.h
 *
 *  Created on: Mar 25, 2026
 *      Author: bens1
 */

#ifndef INC_APP_H_
#define INC_APP_H_


#include "main.h"


/* Wrappers to access MX gernerated main.c functions */
void nonsecure_init(void);
void system_clock_config(void);
void periph_common_clock_config(void);
void mpu_config(void);

int main(void);


#endif /* INC_APP_H_ */