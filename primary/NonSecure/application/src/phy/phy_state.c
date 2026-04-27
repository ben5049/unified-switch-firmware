/*
 * phy_state.c
 *
 *  Created on: 26 Apr 2026
 *      Author: bens1
 */

#include "stdbool.h"
#include "stdint.h"

#include "phy_thread.h"
#include "phy_common.h"
#include "switch_thread.h"
#include "config.h"
#include "utils.h"


#define MAX_TRANSITIONS      (10) /* Used to break livelocks with a 0 latency loop in the state machine (shouldn't be possible) */

#define PHY_INDEX(h)         (((phy_info_t *) (((phy_handle_base_t *) (h))->callback_context))->index)
#define PHY_STATE(h)         (((phy_info_t *) (((phy_handle_base_t *) (h))->callback_context))->connection_state)
#define PHY_NEXT_UPDATE(h)   (((phy_info_t *) (((phy_handle_base_t *) (h))->callback_context))->next_update_time)
#define PHY_LINK_ATTEMPTS(h) (((phy_info_t *) (((phy_handle_base_t *) (h))->callback_context))->link_attempts)
#define PHY_LAST_TEST(h)     (((phy_info_t *) (((phy_handle_base_t *) (h))->callback_context))->last_self_test_time)


static phy_status_t phy_state_update(phy_handle_base_t *hphy, uint32_t current_time) {

    phy_status_t           status        = PHY_OK;
    phy_connection_state_t current_state = PHY_STATE(hphy);
    phy_connection_state_t next_state    = current_state;
    uint32_t               interval      = 0; /* How long to spend in next_state before updating again */
    uint8_t                transitions   = 0;

    /* Process state machine transitions, if the interval between transitions is 0 ms then process them back to back */
    while ((interval == 0) && (transitions < MAX_TRANSITIONS)) {

        switch (current_state) {

            case PHY_STATE_UNINITIALISED: {
                if (hphy->initialised) {
                    next_state = PHY_STATE_INITIALISED;
                } else {
                    next_state = current_state;
                    interval   = 300;
                }
                break;
            }

            case PHY_STATE_INITIALISED: {

                /* Check link state
                 * This is needed because the link can go up before the interrupts are enabled */
                if (!hphy->linkup) {
                    status = PHY_GetLinkState(hphy, NULL);
                    if (status != PHY_OK) goto end;
                }

                if (hphy->linkup) {
                    next_state = PHY_STATE_CONNECTED;
                }

                else {
                    next_state = PHY_STATE_RECONNECT;
                }

                break;
            }

            case PHY_STATE_WAIT_FOR_LINK: {

                /* Check link state in case an interrupt was missed */
                if (!hphy->linkup) {
                    status = PHY_GetLinkState(hphy, NULL);
                    if (status != PHY_OK) goto end;
                }

                /* The link is up */
                if (hphy->linkup) {
                    next_state              = PHY_STATE_CONNECTED;
                    PHY_LINK_ATTEMPTS(hphy) = 0; /* Reset the counter */
                }

                /* No link and we have run out of attempts */
                else if (PHY_LINK_ATTEMPTS(hphy) > PHY_WAITING_FOR_LINK_ATTEMPTS) {
                    next_state              = PHY_STATE_SLEEP_START;
                    PHY_LINK_ATTEMPTS(hphy) = 0; /* Reset the counter */
                }

                else {

                    /* Perform a self test periodically (or at startup) */
                    if ((PHY_SELF_TEST_ON_STARTUP && (PHY_LAST_TEST(hphy) == 0)) ||
                        (current_time - PHY_LAST_TEST(hphy)) > PHY_SELF_TEST_INTERVAL) {
                        next_state = PHY_STATE_SELF_TEST_1;
                    }

                    /* Increment counter, wait, and try again */
                    else {
                        next_state               = current_state;
                        interval                 = PHY_WAITING_FOR_LINK_INTERVAL;
                        PHY_LINK_ATTEMPTS(hphy) += 1;
                    }
                }

                break;
            }

            case PHY_STATE_CONNECTED: {

                /* Check link state in case an interrupt was missed */
                if (hphy->linkup) {
                    status = PHY_GetLinkState(hphy, NULL);
                    if (status != PHY_OK) goto end;
                }

                /* Link has gone down */
                if (!hphy->linkup) {
                    next_state = PHY_STATE_RECONNECT;
                }

                /* Still connected */
                else {

                    uint8_t sqi = PHY_SQI_INVALID;

                    /* Get the signal quality index (0-100) */
                    status = PHY_GetSQI(hphy, &sqi);
                    if (status != PHY_OK) goto end;

                    next_state = current_state;
                    interval   = 1000;
                }

                break;
            }

            /* Attempt to quickly reconnect */
            case PHY_STATE_RECONNECT: {

                /* Check link state in case an interrupt was missed */
                if (!hphy->linkup) {
                    status = PHY_GetLinkState(hphy, NULL);
                    if (status != PHY_OK) goto end;
                }

                /* The link is up */
                if (hphy->linkup) {
                    next_state              = PHY_STATE_CONNECTED;
                    PHY_LINK_ATTEMPTS(hphy) = 0; /* Reset the counter */
                }

                /* No link and we have run out of reconnect attempts */
                else if (PHY_LINK_ATTEMPTS(hphy) > PHY_RECONNECT_ATTEMPTS) {
                    next_state              = PHY_STATE_WAIT_FOR_LINK;
                    PHY_LINK_ATTEMPTS(hphy) = 0; /* Reset the counter */
                }

                /* Increment counter, wait, and try again */
                else {
                    next_state               = current_state;
                    interval                 = PHY_RECONNECT_INTERVAL;
                    PHY_LINK_ATTEMPTS(hphy) += 1;
                }

                break;
            }

            /* Sleep */
            case PHY_STATE_SLEEP_START: {

                /* Disable traffic to the switch port */
                if (SJA1105_PortSetForwarding(
                        phy_to_switch_handle(PHY_INDEX(hphy)),
                        phy_to_switch_port(PHY_INDEX(hphy)),
                        false) != SJA1105_OK) {
                    status = PHY_ERROR;
                    goto end;
                }

                /* Let the traffic drain out */
                tx_thread_sleep_ms(1);

                /* Sleep the PHY */
                status = PHY_Sleep(hphy);
                if ((status != PHY_OK)) goto end;

                /* Sleep the switch port (turn off clocks) */
                // TODO: re-enable when implemented
                // if (SJA1105_PortSleep(
                //         phy_to_switch_handle(PHY_INDEX(hphy)),
                //         phy_to_switch_port(PHY_INDEX(hphy))) != SJA1105_OK) {
                //     status = PHY_ERROR;
                //     goto end;
                // }

                next_state = PHY_STATE_SLEEP_STOP;
                interval   = PHY_SLEEP_INTERVAL;
                break;
            }

            /* Wake up */
            case PHY_STATE_SLEEP_STOP: {

                /* Wake the switch port (turn on clocks) */
                // TODO: re-enable when implemented
                // if (SJA1105_PortWake(
                //         phy_to_switch_handle(PHY_INDEX(hphy)),
                //         phy_to_switch_port(PHY_INDEX(hphy))) != SJA1105_OK) {
                //     status = PHY_ERROR;
                //     goto end;
                // }

                /* Wake the PHY */
                status = PHY_Wake(hphy);
                if ((status != PHY_OK)) goto end;

                /* Enable traffic to the switch port */
                if (SJA1105_PortSetForwarding(
                        phy_to_switch_handle(PHY_INDEX(hphy)),
                        phy_to_switch_port(PHY_INDEX(hphy)),
                        true) != SJA1105_OK) {
                    status = PHY_ERROR;
                    goto end;
                }

                next_state = PHY_STATE_WAIT_FOR_LINK;
                break;
            }

            case PHY_STATE_SELF_TEST_1: {

                PHY_LAST_TEST(hphy) = current_time;

                /* Tests only available on certain PHYs */
                if ((hphy->config.variant == PHY_VARIANT_88Q2112) || (hphy->config.variant == PHY_VARIANT_88Q2110)) {

                    /* Start a built-in self test (BIST) */
                    status = PHY_88Q211X_Start1000MBIST((phy_handle_88q211x_t *) hphy);
                    if (status != PHY_OK) goto end;

                    next_state = PHY_STATE_SELF_TEST_2;
                    interval   = 100;
                }

                /* No self test 1 available, go back to waiting for link */
                else {
                    next_state = PHY_STATE_WAIT_FOR_LINK;
                    interval   = PHY_WAITING_FOR_LINK_INTERVAL;
                }

                break;
            }

            case PHY_STATE_SELF_TEST_2: {

                /* Get BIST results and start a virtual cable test (VCT) */
                if ((hphy->config.variant == PHY_VARIANT_88Q2112) || (hphy->config.variant == PHY_VARIANT_88Q2110)) {

                    bool error = false;

                    /* Get the BIST results (this also stops it) */
                    status = PHY_88Q211X_Get1000MBISTResults((phy_handle_88q211x_t *) hphy, &error);
                    if (status != PHY_OK) goto end;
                    if (error) {
                        next_state = PHY_STATE_ERROR_RECOVERABLE;
                        break;
                    }

                    /* Start a virtual cable test */
                    status = PHY_88Q211X_StartVCT((phy_handle_88q211x_t *) hphy);
                    if ((status != PHY_OK)) goto end;

                    next_state = PHY_STATE_SELF_TEST_3;
                    interval   = PHY_88Q211X_VCT_LENGTH; /* Wait for the test to complete */
                }

                /* No self test 2 available, go back to waiting for link */
                else {
                    next_state = PHY_STATE_WAIT_FOR_LINK;
                    interval   = PHY_WAITING_FOR_LINK_INTERVAL;
                }

                break;
            }

            case PHY_STATE_SELF_TEST_3: {

                /* Get VCT results */
                if ((hphy->config.variant == PHY_VARIANT_88Q2112) || (hphy->config.variant == PHY_VARIANT_88Q2110)) {

                    phy_cable_state_88q211x_t cable_state           = PHY_CABLE_STATE_88Q211X_OPEN;
                    uint32_t                  maximum_peak_distance = 0;

                    /* Get the VCT results and stop the test */
                    status = PHY_88Q211X_GetVCTResults((phy_handle_88q211x_t *) hphy, &cable_state, &maximum_peak_distance);
                    if (status == PHY_TIMEOUT) {
                        LOG_WARNING("PHY%d VCT Didn't finish in %d ms", PHY_INDEX(hphy), PHY_88Q211X_VCT_LENGTH);
                    } else if (status != PHY_OK) {
                        goto end;
                    }
                    status = PHY_88Q211X_StopVCT((phy_handle_88q211x_t *) hphy);
                    if ((status != PHY_OK)) goto end;
                }

                /* All tests done, go back to waiting for link */
                next_state = PHY_STATE_WAIT_FOR_LINK;
                interval   = PHY_WAITING_FOR_LINK_INTERVAL;
                break;
            }

            /* No PHYs use these states */
            case PHY_STATE_SELF_TEST_4:
            case PHY_STATE_SELF_TEST_5:
            case PHY_STATE_SELF_TEST_6:
            case PHY_STATE_SELF_TEST_7:
            case PHY_STATE_SELF_TEST_8:
            case PHY_STATE_SELF_TEST_9:
            case PHY_STATE_SELF_TEST_10: {

                /* Should be unreachable, go back to waiting for link */
                next_state = PHY_STATE_WAIT_FOR_LINK;
                interval   = PHY_WAITING_FOR_LINK_INTERVAL;
                break;
            }

            case PHY_STATE_ERROR_RECOVERABLE: {

                /* TODO: Attempt to recover */

                next_state = PHY_STATE_ERROR_UNRECOVERABLE;
                break;
            }

            case PHY_STATE_ERROR_UNRECOVERABLE: {
                next_state = current_state;
                interval   = 60000; /* Large number: don't need to bother checking again since this state is a dead end */
                break;
            }

            default: {
                status = PHY_PARAMETER_ERROR;
                goto end;
                break;
            }
        }

        current_state = next_state;
        transitions++;
    }

#if DEBUG
    if (transitions >= MAX_TRANSITIONS) {
        status = PHY_SW_DEADLOCK;
        goto end;
    }
#endif

    PHY_STATE(hphy)       = next_state;
    PHY_NEXT_UPDATE(hphy) = current_time + interval;

end:

    return status;
}


/* Update the state machine if the required time has elapsed */
phy_status_t phy_state_update_poll(phy_handle_base_t *hphy, uint32_t current_time) {

    phy_status_t status = PHY_OK;

    /* Not ready to update */
    if ((int32_t) (current_time - PHY_NEXT_UPDATE(hphy)) < 0) {
        return status;
    }

    /* Ready to update */
    else {
        return phy_state_update(hphy, current_time);
    }
}


/* Immediately update the state machine regardless of the next update time */
/* Note: this shouldn't be called from an ISR despite the name */
phy_status_t phy_state_update_interrupt(phy_handle_base_t *hphy, uint32_t current_time) {
    return phy_state_update(hphy, current_time);
}
