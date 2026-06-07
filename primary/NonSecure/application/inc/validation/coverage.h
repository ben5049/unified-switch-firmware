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

#if VALIDATION_PTP_TX

    VAL_COVER_ARRAY_DECLARE(PTP_TX, PACKET_SENT, NUM_PORTS);
    VAL_COVER_ARRAY_DECLARE(PTP_TX, PACKET_DROPPED, NUM_PORTS);
    VAL_COVER_DECLARE(PTP_TX, PACKET_TOO_SHORT);
    VAL_COVER_DECLARE(PTP_TX, PACKET_INVALID_PORT);
    VAL_COVER_ARRAY_DECLARE(PTP_TX, TS_RECEIVED, NUM_PORTS);
    VAL_COVER_ARRAY_DECLARE(PTP_TX, TS_MISSED, NUM_PORTS);
    VAL_COVER_DECLARE(PTP_TX, DRIVER_ERROR);       /* Failed to send packet */
    VAL_COVER_DECLARE(PTP_TX, SLOW);               /* Took multiple polls for the management route to free */
    VAL_COVER_DECLARE(PTP_TX, MGMT_ROUTE_TIMEOUT); /* Management route was never freed */

#endif

#if VALIDATION_PTP_RX

    VAL_COVER_DECLARE(PTP_RX, META);                    /* Number of META frames filtered */
    VAL_COVER_DECLARE(PTP_RX, PACKET);                  /* Number of PTP packets filtered */
    VAL_COVER_DECLARE(PTP_RX, EVENT);                   /* Number of event PTP packets received (SYNC, PDELAY_REQ, PDELAY_RESP) */
    VAL_COVER_DECLARE(PTP_RX, GENERAL);                 /* Number of general PTP packets received */
    VAL_COVER_ARRAY_DECLARE(PTP_RX, PORT, NUM_PORTS);   /* Number of packets received on each port */
    VAL_COVER_DECLARE(PTP_RX, NO_META);                 /* Expected a META frame but didn't get one */
    VAL_COVER_DECLARE(PTP_RX, META_DELAYED_BY_1);       /* Number of META frames that had 1 frame between them and the packets that generated them */
    VAL_COVER_DECLARE(PTP_RX, META_DELAYED_BY_1_TWICE); /* Number of times META_DELAYED_BY_1 happens back to back. Represents a desync */
    VAL_COVER_DECLARE(PTP_RX, LONE_META);               /* Received a META frame but wasn't expecting one */
    VAL_COVER_DECLARE(PTP_RX, INVALID_VLAN);            /* Number of packets with invalid VLANs received */
    VAL_COVER_DECLARE(PTP_RX, META_DROP);
    VAL_COVER_DECLARE(PTP_RX, PACKET_DROP);

    VAL_FAULT_DECLARE(PTP_RX, FILTER_DROP_META);
    VAL_FAULT_DECLARE(PTP_RX, FILTER_DROP_PTP);

#endif

#if VALIDATION_PTP_EVENT

    VAL_COVER_DECLARE(PTP_EVENT, SYNC);
    VAL_COVER_DECLARE(PTP_EVENT, MASTER_NEW);
    VAL_COVER_DECLARE(PTP_EVENT, MASTER_TIMEOUT);
    VAL_COVER_DECLARE(PTP_EVENT, MASTER_SYNC_TIMEOUT);

    VAL_FAULT_DECLARE(PTP_EVENT, SYNC_DROP);

#endif

#if VALIDATION_PTP_CLOCK

    VAL_COVER_DECLARE(PTP_CLOCK, CB_SET);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_GET);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_ADJUST);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_SET_FULL);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_TS_EXTRACT);
    VAL_COVER_DECLARE(PTP_CLOCK, QUEUE_FULL);
    VAL_COVER_DECLARE(PTP_CLOCK, INVALID_SEQUENCE);
    VAL_COVER_DECLARE(PTP_CLOCK, SWITCH_ADJUST_COARSE);
    VAL_COVER_DECLARE(PTP_CLOCK, SWITCH_ADJUST_FINE);
    VAL_COVER_DECLARE(PTP_CLOCK, MAC_SYNC_FAILED);
    VAL_COVER_DECLARE(PTP_CLOCK, MAC_SYNC_ADJUST_COARSE);
    VAL_COVER_DECLARE(PTP_CLOCK, MAC_SYNC_ADJUST_FINE);

    VAL_FAULT_DECLARE(PTP_CLOCK, THREAD_LAG); /* Lag injected into clock thread processing */

#endif

#if VALIDATION_SWITCH_UTILS

    VAL_FAULT_DECLARE(SWITCH_UTILS, RATE_SET_PREEMPT);

#endif
};


extern struct coverage_s VALIDATION_COVER_STRUCT;


#ifdef __cplusplus
}
#endif

#endif /* INC_COVERAGE_H_ */
