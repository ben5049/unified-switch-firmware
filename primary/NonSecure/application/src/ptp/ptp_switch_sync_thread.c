/*
 * ptp_switch_sync_thread.c
 *
 *  Created on: May 31, 2026
 *      Author: bens1
 */

#include "app.h"
#include "ptp.h"
#include "switch.h"
#include "utils.h"


// TODO: could this be integrated into clock thread?


#if NUM_SWITCHES > 1


/* This thread synchronises the SJA1105 clocks with each other */
void ptp_switch_sync_thread_entry(uint32_t initial_input) {

    // tx_status_t tx_status = TX_SUCCESS;
    // sja1105_status_t switch_status = SJA1105_OK;

    // uint32_t event_flags;

    // NX_PTP_TIME offset;

    // uint32_t new_ptp_clock_rate;
    // uint32_t ptp_clock_rates[NUM_SWITCHES];
    // uint8_t  skip_switch_sync[NUM_SWITCHES];

    /* Reset arrays */
    // for (switch_index_t i = SWITCH0; i < NUM_SWITCHES; i++) {
    //     ptp_clock_rates[i]  = SJA1105_PTP_CLK_RATE_DEFAULT;
    //     skip_switch_sync[i] = 0;
    // }

    // while (1) {

    //     /* Wait for an event from the timer */
    //     tx_status = tx_event_flags_get(
    //         &ptp_clock_events_group,
    //         PTP_CLOCK_EVENT_SWITCH_SYNC,
    //         TX_OR_CLEAR,
    //         &event_flags,
    //         TX_WAIT_FOREVER);
    //     TX_CHECK(tx_status);

    //     assert(event_flags == PTP_CLOCK_EVENT_SWITCH_SYNC);

    //     /* This is agnostic to the number of switches in the system */
    //     for (switch_index_t i = SWITCH1; i < NUM_SWITCHES; i++) {

    //         /* After reaching equilibrium skip PTP_SWITCH_SYNC_SKIP syncs since they aren't necessary */
    //         if (skip_switch_sync[i] > 0) {
    //             skip_switch_sync[i]--;
    //             continue;
    //         }

    //         /* Get the offset between the two switches */
    //         switch_status = switch_get_timestamp_offsets(SWITCH0, i, &offset);
    //         SWITCH_CHECK(switch_status);

    //         assert(offset.second_high == 0);
    //         assert(offset.second_low == 0);

    //         /* Adjust the PTP clock rate to sync slave with master within +-3
    //          * ticks (+-24ns).
    //          *
    //          * Note: The switches share a common oscillator so they tick at
    //          *       the same rate, the only thing that can cause offsets is
    //          *       jitter when the application adjusts their rates.
    //          */
    //         if (offset.nanosecond > (SJA1105_NS_PER_TS_TICK * 6)) {
    //             new_ptp_clock_rate = SJA1105_PTP_CLK_RATE_SLIGHTLY_FASTER;
    //         } else if (offset.nanosecond < -(SJA1105_NS_PER_TS_TICK * 6)) {
    //             new_ptp_clock_rate = SJA1105_PTP_CLK_RATE_SLIGHTLY_SLOWER;
    //         } else {
    //             new_ptp_clock_rate  = SJA1105_PTP_CLK_RATE_DEFAULT;
    //             skip_switch_sync[i] = PTP_SWITCH_SYNC_SKIP;
    //         }

    //         /* Write the new rate */
    //         if (new_ptp_clock_rate != ptp_clock_rates[i]) {
    //             ptp_clock_rates[i] = new_ptp_clock_rate;
    //             switch_status      = SJA1105_SetPTPClockRate(&switch_handles[i], ptp_clock_rates[i]);
    //             SWITCH_CHECK(switch_status);
    //             LOG_INFO("PTP: Switch %d current offset = %li ns", i, offset.nanosecond);
    //         }
    //     }
    // }
}

#endif /* NUM_SWITCHES > 1 */
