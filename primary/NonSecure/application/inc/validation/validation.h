/*
 * validation.h
 *
 *  Created on: 5 Jun 2026
 *      Author: bens1
 */

#ifndef INC_VALIDATION_H_
#define INC_VALIDATION_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdatomic.h"

#include "app.h"
#include "secure_nsc.h"


#ifndef VAL_SEED
#define VAL_SEED (0) /* 0 Means random seed */
#endif

#define VAL_CONCAT(a, b) a##b
#define _VAL_EVAL(a, b)  VAL_CONCAT(a, b)

#define _VAL_BASE_1(...) \
    do {                 \
        __VA_ARGS__;     \
    } while (0)
#define _VAL_BASE_0(...) \
    do {                 \
    } while (0)

#if VALIDATION_ENABLE
#define VAL_BASE(unit, ...) _VAL_EVAL(_VAL_BASE_, VALIDATION_##unit)(__VA_ARGS__)
#else
#define VAL_BASE(unit, ...) \
    do {                    \
    } while (0)
#endif

#define VAL_INIT()                 VAL_BASE(ENABLE, validation_init())

#define VAL_COVER_NAME(unit, item) coverage.unit##_##item
#define VAL_COVER(unit, item)      VAL_BASE(unit, VAL_COVER_NAME(unit, item)++)

#define _VAL_CHANCE(unit, item, chance, logic)  \
    do {                                        \
        if (validation_rand_u32() < (chance)) { \
            VAL_COVER(unit, item);              \
            logic;                              \
        }                                       \
    } while (0)

#define VAL_CHANCE(unit, item, chance, logic)          VAL_BASE(unit, _VAL_CHANCE(unit, item, chance, logic))

#define VAL_EARLY_RETURN(unit, item, chance, ret)      VAL_CHANCE(unit, item, chance, return (ret))
#define VAL_INJECT_VALUE(unit, item, chance, var, val) VAL_CHANCE(unit, item, chance, (var) = (val))


/* Possible chance values */
#define VAL_NEVER      (0)
#define VAL_ALWAYS     (UINT32_MAX)
#define VAL_1_IN_10    (UINT32_MAX / 10)
#define VAL_1_IN_100   (UINT32_MAX / 100)
#define VAL_1_IN_1000  (UINT32_MAX / 1000)
#define VAL_1_IN_10000 (UINT32_MAX / 10000)


typedef struct {

#if VALIDATION_PTP
    atomic_uint_fast32_t PTP_RX_FILTER_DROP_META;
    atomic_uint_fast32_t PTP_RX_FILTER_DROP_PTP;
#endif

} validation_coverage_t;


extern validation_coverage_t coverage;


void     validation_init(void);
uint32_t validation_rand_u32(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_VALIDATION_H_ */
