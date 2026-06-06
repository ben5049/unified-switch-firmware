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


/* ---------------------------------------------------------------------------- */
/* Public Macros */
/* ---------------------------------------------------------------------------- */

#ifndef VALIDATION_SEED
#define VALIDATION_SEED (0) /* 0 Means random seed */
#endif

#ifndef VALIDATION_FAULT_INJECTION
#define VALIDATION_FAULT_INJECTION (0) /* Disabled by default */
#endif

#define VAL_INIT()                                     _VAL_BASE(ENABLE, validation_init())
#define VAL_COVER(unit, item)                          _VAL_BASE(unit, _VAL_COVER_NAME(unit, item)++)
#define VAL_CHANCE(unit, item, chance, logic)          _VAL_BASE(unit, _VAL_CHANCE(unit, item, chance, logic))
#define VAL_EARLY_RETURN(unit, item, chance, ret)      VAL_CHANCE(unit, item, chance, return (ret))
#define VAL_INJECT_VALUE(unit, item, chance, var, val) VAL_CHANCE(unit, item, chance, (var) = (val))
#define VAL_EARLY_BREAK(unit, item, chance)            ({bool _b = false; VAL_INJECT_VALUE(unit, item, chance, _b, true); if (_b) break; })

/* Possible chance values */
#define VAL_NEVER      (0)
#define VAL_ALWAYS     (UINT32_MAX)
#define VAL_1_IN_10    (UINT32_MAX / 10)
#define VAL_1_IN_100   (UINT32_MAX / 100)
#define VAL_1_IN_1000  (UINT32_MAX / 1000)
#define VAL_1_IN_10000 (UINT32_MAX / 10000)

/* ---------------------------------------------------------------------------- */
/* Private Macros */
/* ---------------------------------------------------------------------------- */

#define _VAL_CONCAT(a, b) a##b
#define _VAL_EVAL(a, b)   _VAL_CONCAT(a, b)

#define _VAL_BASE_1(...) \
    do {                 \
        __VA_ARGS__;     \
    } while (0)
#define _VAL_BASE_0(...) \
    do {                 \
    } while (0)

/* _VAL_BASE Must be used by all public macros */
#if VALIDATION_ENABLE
#define _VAL_BASE(unit, ...) _VAL_EVAL(_VAL_BASE_, VALIDATION_##unit)(__VA_ARGS__)
#else
#define _VAL_BASE(unit, ...) \
    do {                     \
    } while (0)
#endif

#define _VAL_CHANCE(unit, item, chance, logic)                                  \
    do {                                                                        \
        if (VALIDATION_FAULT_INJECTION && (validation_rand_u32() < (chance))) { \
            VAL_COVER(unit, item);                                              \
            logic;                                                              \
        }                                                                       \
    } while (0)

#define _VAL_COVER_NAME(unit, item) coverage.unit##_##item


typedef struct {

#if VALIDATION_PTP

    /* Normal covers */
    atomic_uint_fast32_t PTP_MASTER_SYNC_TIMEOUT;

    /* Fault injection covers */
    atomic_uint_fast32_t PTP_FAULT_RX_FILTER_DROP_META;
    atomic_uint_fast32_t PTP_FAULT_RX_FILTER_DROP_PTP;
    atomic_uint_fast32_t PTP_FAULT_MASTER_SYNC_TIMEOUT;

#endif

} validation_coverage_t;


extern validation_coverage_t coverage;


void     validation_init(void);
uint32_t validation_rand_u32(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_VALIDATION_H_ */
