/*
 * encodings.h
 *
 *  Created on: Oct 2, 2025
 *      Author: bens1
 */

#ifndef INC_ENCODINGS_H_
#define INC_ENCODINGS_H_


#define ENCODING_HEARTBEAT    "application/protobuf;Heartbeat"
#define ENCODING_SWITCH_STATS "application/protobuf;SwitchDiag"

#define PB_SET_FIELD(struct, field, value) \
    do {                                   \
        struct.field       = (value);      \
        struct.has_##field = true;         \
    } while (0)


#endif /* INC_ENCODINGS_H_ */
