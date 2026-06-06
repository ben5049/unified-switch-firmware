/*
 * coverage.h
 *
 *  Created on: 6 Jun 2026
 *      Author: bens1
 */

#ifndef INC_COVERAGE_H_
#define INC_COVERAGE_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "validation.h"


struct coverage_s {

#if VALIDATION_PTP

    /* Normal covers */
    VAL_COVER_DECLARE(PTP, MASTER_SYNC_TIMEOUT);

    /* Fault injection covers */
    VAL_FAULT_DECLARE(PTP, RX_FILTER_DROP_META);
    VAL_FAULT_DECLARE(PTP, RX_FILTER_DROP_PTP);
    VAL_FAULT_DECLARE(PTP, MASTER_SYNC_TIMEOUT);

#endif
};


extern struct coverage_s VALIDATION_COVER_STRUCT;


#ifdef __cplusplus
}
#endif

#endif /* INC_COVERAGE_H_ */
