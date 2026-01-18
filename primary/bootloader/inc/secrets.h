/*
 * secrets.h
 *
 *  Created on: Aug 22, 2025
 *      Author: bens1
 */

#ifndef INC_SECRETS_H_
#define INC_SECRETS_H_


#include "stdint.h"


void set_ecdsa_key_x(uint8_t *pPubKeyCurvePtX);     /* 32-byte array */
void set_ecdsa_key_y(uint8_t *pPubKeyCurvePtY);     /* 32-byte array */

void set_saes_key(uint32_t *pKeySAES);              /* 4-word array */
void set_saes_init_vector(uint32_t *pInitVectSAES); /* 4-word array */


#endif /* INC_SECRETS_H_ */
