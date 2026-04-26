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
                        next_state = PHY_STATE_SELF_TEST;
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
                status = PHY_Sleep(hphy);
                if ((status != PHY_OK)) goto end;
                next_state = PHY_STATE_SLEEP_STOP;
                interval   = PHY_SLEEP_INTERVAL;
                break;
            }

            /* Wake up */
            case PHY_STATE_SLEEP_STOP: {
                status = PHY_Wake(hphy);
                if ((status != PHY_OK)) goto end;
                next_state = PHY_STATE_WAIT_FOR_LINK;
                break;
            }

            case PHY_STATE_SELF_TEST: {

                PHY_LAST_TEST(hphy) = current_time;

                /* TODO: Perform a self test */

                next_state = PHY_STATE_WAIT_FOR_LINK;
                interval   = PHY_WAITING_FOR_LINK_INTERVAL;
                break;
            }

            case PHY_STATE_ERROR_RECOVERABLE: {

                /* TODO: Attempt to recover */

                next_state = PHY_STATE_WAIT_FOR_LINK;
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
