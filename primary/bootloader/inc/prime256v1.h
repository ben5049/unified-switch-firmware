/*
 * prime256v1.h
 *
 *  Created on: Aug 20, 2025
 *      Author: bens1
 */

#ifndef INC_PRIME256V1_H_
#define INC_PRIME256V1_H_


#include "stdint.h"


#define ECDSA_SIZE 32


extern const uint8_t  prime256v1_Prime[];
extern const uint32_t prime256v1_Prime_len;

extern const uint8_t prime256v1_A[];

/* PKA operation need abs(a) */
extern const uint8_t  prime256v1_absA[];
extern const uint32_t prime256v1_A_len;

/* PKA operation need the sign of A */
extern const uint32_t prime256v1_A_sign;
extern const uint8_t  prime256v1_B[];
extern const uint32_t prime256v1_B_len;
extern const uint8_t  prime256v1_Generator[];
extern const uint32_t prime256v1_Generator_len;

/* This buffer is extracted from prime256v1_Generator as its first part */
extern const uint8_t  prime256v1_GeneratorX[];
extern const uint32_t prime256v1_GeneratorX_len;

/* This buffer is extracted from prime256v1_Generator as its second part */
extern const uint8_t  prime256v1_GeneratorY[];
extern const uint32_t prime256v1_GeneratorY_len;
extern const uint8_t  prime256v1_Order[];
extern const uint32_t prime256v1_Order_len;
extern const uint32_t prime256v1_Cofactor; /* (0x1) */
extern const uint8_t  prime256v1_Seed[];
extern const uint32_t prime256v1_Seed_len;


#endif /* INC_PRIME256V1_H_ */
