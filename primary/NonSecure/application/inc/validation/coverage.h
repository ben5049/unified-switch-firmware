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

    VAL_COVER_ARRAY_DECLARE(PTP_TX, PACKET_SENT, NUM_PHYS);
    VAL_COVER_ARRAY_DECLARE(PTP_TX, PACKET_DROPPED, NUM_PHYS);
    VAL_COVER_DECLARE(PTP_TX, PACKET_INVALID_PORT);
    VAL_COVER_ARRAY_DECLARE(PTP_TX, TS_RECEIVED, NUM_PHYS);
    VAL_COVER_ARRAY_DECLARE(PTP_TX, TS_MISSED, NUM_PHYS);
    VAL_COVER_DECLARE(PTP_TX, DRIVER_ERROR);       /* Failed to send packet */
    VAL_COVER_DECLARE(PTP_TX, SLOW);               /* Took multiple polls for the management route to free */
    VAL_COVER_DECLARE(PTP_TX, MGMT_ROUTE_TIMEOUT); /* Management route was never freed */

#endif

#if VALIDATION_PTP_RX

    VAL_COVER_DECLARE(PTP_RX, META);                 /* Number of META frames filtered */
    VAL_COVER_DECLARE(PTP_RX, PACKET);               /* Number of PTP packets filtered */
    VAL_COVER_DECLARE(PTP_RX, EVENT);                /* Number of event PTP packets received (SYNC, PDELAY_REQ, PDELAY_RESP) */
    VAL_COVER_DECLARE(PTP_RX, GENERAL);              /* Number of general PTP packets received */
    VAL_COVER_ARRAY_DECLARE(PTP_RX, PORT, NUM_PHYS); /* Number of packets received on each port */
    VAL_COVER_DECLARE(PTP_RX, NO_META);              /* Expected a META frame but didn't get one */
    VAL_COVER_DECLARE(PTP_RX, LONE_META);            /* Received a META frame but wasn't expecting one */
    VAL_COVER_DECLARE(PTP_RX, INVALID_VLAN);         /* Number of packets with invalid VLANs received */
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

    VAL_COVER_DECLARE(PTP_CLOCK, CB_INIT);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_SET);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_GET);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_ADJUST);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_TS_EXTRACT);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_TS_PREPARE);
    VAL_COVER_DECLARE(PTP_CLOCK, CB_INVALID);
    VAL_COVER_DECLARE(PTP_CLOCK, MAC_SYNC_FAILED);

#endif
};


extern struct coverage_s VALIDATION_COVER_STRUCT;


#ifdef __cplusplus
}
#endif

#endif /* INC_COVERAGE_H_ */
