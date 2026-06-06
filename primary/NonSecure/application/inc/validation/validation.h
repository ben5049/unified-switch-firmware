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

#ifndef VALIDATION_COVER_STRUCT
#define VALIDATION_COVER_STRUCT coverage
#endif

/* Generic macros */
#define VAL_INIT()      _VAL_BASE(ENABLE, validation_init()) /* Initialise validation (reset coverage, seed RNG, etc) */
#define VAL_TERMINATE() _VAL_BASE(ENABLE, error_handler())   /* Stop execution so the state can be inspected */

/* Coverage macros */
#define VAL_COVER_DECLARE(unit, item) atomic_uint_fast32_t _VAL_COVER_NAME(unit, item)                       /* Add item to VALIDATION_COVER_STRUCT */
#define VAL_COVER(unit, item)         _VAL_BASE(unit, VALIDATION_COVER_STRUCT._VAL_COVER_NAME(unit, item)++) /* Cover an item */

/* Fault injection macros */
#define VAL_FAULT_DECLARE(unit, item)                  atomic_uint_fast32_t _VAL_FAULT_NAME(unit, item)              /* Add item to VALIDATION_COVER_STRUCT */
#define VAL_FAULT_CHANCE(unit, item, chance, logic)    _VAL_BASE(unit, _VAL_FAULT_CHANCE(unit, item, chance, logic)) /* Random chance of logic being executed */
#define VAL_FAULT_RETURN(unit, item, chance, ret)      VAL_FAULT_CHANCE(unit, item, chance, return (ret))            /* Random chance of returning */
#define VAL_FAULT_MODIFY(unit, item, chance, var, val) VAL_FAULT_CHANCE(unit, item, chance, (var) = (val))           /* Random chance of var being modified to val */
#define VAL_FAULT_BREAK(unit, item, chance)            ({bool _b = false; VAL_FAULT_MODIFY(unit, item, chance, _b, true); if (_b) break; })                                                         /* Random chance of breaking */

/* Chance constants */
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

#define _VAL_FAULT_CHANCE(unit, item, chance, logic)                                \
    do {                                                                            \
        if (VALIDATION_FAULT_INJECTION && (validation_rand_u32() < (chance))) {     \
            _VAL_BASE(unit, VALIDATION_COVER_STRUCT._VAL_FAULT_NAME(unit, item)++); \
            logic;                                                                  \
        }                                                                           \
    } while (0)

#define _VAL_COVER_NAME(unit, item) unit##_COVER_##item
#define _VAL_FAULT_NAME(unit, item) unit##_FAULT_##item


void     validation_init(void);
uint32_t validation_rand_u32(void);


/* Include application specific coverage definitions */
#include "coverage.h"


#ifdef __cplusplus
}
#endif

#endif /* INC_VALIDATION_H_ */
