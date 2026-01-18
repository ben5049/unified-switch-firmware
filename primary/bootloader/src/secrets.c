/*
 * secrets.c
 *
 *  Created on: Aug 22, 2025
 *      Author: bens1
 */

#include "secrets.h"
#include "prime256v1.h"


static const uint8_t  ecsda_key_x[ECDSA_SIZE] = {0x35, 0x50, 0xac, 0x3d, 0x6f, 0x48, 0x7e, 0x21,
                                                 0x8c, 0xb2, 0xa5, 0x56, 0xa8, 0x8c, 0x5a, 0xf9,
                                                 0x1c, 0xe0, 0xe0, 0x7e, 0x35, 0x49, 0xa9, 0xe2,
                                                 0xd5, 0xf4, 0xac, 0xc3, 0xfa, 0x42, 0x8b, 0x63};
static const uint8_t  ecsda_key_y[ECDSA_SIZE] = {0x27, 0xd8, 0x68, 0x9a, 0x51, 0x6a, 0x98, 0x4e,
                                                 0x3e, 0xbe, 0x94, 0xff, 0xb7, 0x73, 0xfd, 0x35,
                                                 0x8e, 0x62, 0x87, 0x5c, 0xd5, 0x0f, 0x73, 0x7d,
                                                 0xe5, 0xd5, 0x71, 0x68, 0xe5, 0x16, 0x3d, 0xb1};
static const uint32_t saes_key[4]             = {0xd377d3e5, 0x8c577eca, 0x766ecbab, 0x5844cb45};
static const uint32_t saes_init_vector[4]     = {0x020e71fa, 0x8748cf24, 0x9dd11057, 0x9ecacbd0};


void set_ecdsa_key_x(uint8_t *pPubKeyCurvePtX) {
    for (uint_fast8_t i = 0; i < ECDSA_SIZE; i++) {
        pPubKeyCurvePtX[i] = ecsda_key_x[i];
    }
}


void set_ecdsa_key_y(uint8_t *pPubKeyCurvePtY) {
    for (uint_fast8_t i = 0; i < ECDSA_SIZE; i++) {
        pPubKeyCurvePtY[i] = ecsda_key_y[i];
    }
}


void set_saes_key(uint32_t *pKeySAES) {
    for (uint_fast8_t i = 0; i < 4; i++) {
        pKeySAES[i] = saes_key[i];
    }
}

void set_saes_init_vector(uint32_t *pInitVectSAES) {
    for (uint_fast8_t i = 0; i < 4; i++) {
        pInitVectSAES[i] = saes_init_vector[i];
    }
}
