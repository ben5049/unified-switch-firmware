/*
 * validation.c
 *
 *  Created on: 5 Jun 2026
 *      Author: bens1
 */

#include "validation.h"
#include "utils.h"


static uint32_t        seed;
static atomic_uint32_t global_random_number;


void validation_init() {

    assert(DEBUG);
    assert(VALIDATION_ENABLE);

    memset(&VALIDATION_COVER_STRUCT, 0, sizeof(VALIDATION_COVER_STRUCT));

    /* Choose non-zero seed */
    seed = VALIDATION_SEED;
    while (seed == 0) {
        seed = s_random_u32();
    }
    global_random_number = seed;

    LOG_INFO("Validation initialised with seed %lu", seed);
}


uint32_t validation_rand_u32() {

    uint32_t local_random_number = global_random_number;
    uint32_t new_random_number;

    assert(VALIDATION_ENABLE);

    do {
        new_random_number = local_random_number;

        /* Xorshift32 Algorithm */
        new_random_number ^= new_random_number << 13;
        new_random_number ^= new_random_number >> 17;
        new_random_number ^= new_random_number << 5;

    } while (!atomic_compare_exchange_strong(&global_random_number, &local_random_number, new_random_number));

    return new_random_number;
}
