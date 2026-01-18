/*
 * error.h
 *
 *  Created on: Aug 30, 2025
 *      Author: bens1
 */

#ifndef INC_ERROR_H_
#define INC_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"


#define CHECK_STATUS(status, desired, major_error_code)  \
    do {                                                 \
        if ((status) != (desired)) {                     \
            error_handler((major_error_code), (status)); \
        }                                                \
    } while (0)


typedef enum {
    ERROR_GENERIC,
    ERROR_HAL,
    ERROR_LOG,
    ERROR_META,
    ERROR_INTEGRITY,
    ERROR_MEM,
} error_t;


void error_handler(error_t major_error_code, uint8_t minor_error_code);
void nmi_handler(void);


#ifdef __cplusplus
}
#endif

#endif /* INC_ERROR_H_ */
