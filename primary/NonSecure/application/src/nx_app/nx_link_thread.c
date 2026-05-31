/*
 * nx_link_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 *
 */

#include "stdint.h"
#include "stdbool.h"

#include "app.h"
#include "tx_app.h"
#include "nx_app.h"
#include "nx_link_thread.h"
#include "utils.h"
#include "state_machine.h"


#define NULL_ADDRESS 0


/* Private function prototypes */
static void ip_address_change_notify_callback(NX_IP *ip_instance, void *ptr);
#if ENABLE_DHCP_RESTORE
static nx_status_t dhcp_record_set_stored_valid(bool valid);
static nx_status_t dhcp_record_get_stored_valid(bool *valid);
static nx_status_t dhcp_record_restore(NX_DHCP *client);
static nx_status_t dhcp_record_store(NX_DHCP *client);
#endif


uint32_t ip_address;
uint32_t net_mask;

TX_THREAD nx_link_thread_handle;
uint8_t   nx_link_thread_stack[NX_LINK_THREAD_STACK_SIZE];

TX_EVENT_FLAGS_GROUP link_events_handle;

TX_TIMER link_check_timer;
TX_TIMER dhcp_save_timer;


/* This thread monitors the link state */
void nx_link_thread_entry(uint32_t thread_input) {

    nx_status_t nx_status     = NX_SUCCESS;
    tx_status_t tx_status     = TX_SUCCESS;
    uint32_t    actual_status = 0;
    uint32_t    events;
    bool        linkdown = true;

    /* Register the IP address change callback */
    nx_status = nx_ip_address_change_notify(&nx_ip_instance, ip_address_change_notify_callback, NULL);
    NX_CHECK(nx_status);

    /* Start DHCP */
    nx_status = nx_dhcp_start(&dhcp_client);
    NX_CHECK(nx_status);

#if ENABLE_DHCP_RESTORE

    /* Attempt to load and restore the DHCP record on warm boot */
    if (!s_cold_boot()) {
        nx_status = dhcp_record_restore(&dhcp_client);
    } else {
        nx_status = dhcp_record_set_stored_valid(false);
    }
    NX_CHECK(nx_status);

    /* Start the timer to periodically save DHCP record */
    tx_status = tx_timer_activate(&dhcp_save_timer);
    TX_CHECK(tx_status);

#endif

    /* Start the link check timer */
    tx_status = tx_timer_activate(&link_check_timer);
    TX_CHECK(tx_status);

    while (1) {

        /* Get events */
        tx_status = tx_event_flags_get(&link_events_handle, LINK_EVENT_LINK_CHECK | LINK_EVENT_DHCP_SAVE, TX_OR_CLEAR, &events, TX_WAIT_FOREVER);
        TX_CHECK(tx_status);

        /* Check if the link is up */
        if (events & LINK_EVENT_LINK_CHECK) {

            /* Send request to check if the switch is up and running */
            nx_status = nx_ip_interface_status_check(&nx_ip_instance, PRIMARY_INTERFACE, NX_IP_LINK_ENABLED, &actual_status, 0);

            /* The link is up */
            if (nx_status == NX_SUCCESS) {

                /* The link just went up */
                if (linkdown) {

                    linkdown = false;

                    /* Send request to enable our MAC */
                    nx_status = nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_ENABLE, &actual_status);
                    if ((nx_status != NX_SUCCESS) && (nx_status != NX_ALREADY_ENABLED)) error_handler();

                    /* Notify the state machine that the link is up */
                    tx_status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_NX_LINK_UP, TX_OR);
                    TX_CHECK(tx_status);

                    /* Send request to check if an address is resolved */
                    nx_status = nx_ip_interface_status_check(&nx_ip_instance, PRIMARY_INTERFACE, NX_IP_ADDRESS_RESOLVED, &actual_status, 0);

                    /* If we have an IP address then restart DHCP to get a new IP address for the new network */
                    if (nx_status == NX_SUCCESS) {

                        /* Stop DHCP */
                        nx_status = nx_dhcp_stop(&dhcp_client);
                        NX_CHECK(nx_status);

                        /* Re-initialize DHCP */
                        nx_status = nx_dhcp_reinitialize(&dhcp_client);
                        NX_CHECK(nx_status);

                        /* Start DHCP */
                        nx_status = nx_dhcp_start(&dhcp_client);
                        NX_CHECK(nx_status);

#if ENABLE_DHCP_RESTORE

                        /* Attempt to restore the record */
                        nx_status = dhcp_record_restore(&dhcp_client);
                        NX_CHECK(nx_status);

#endif
                    }

                    /* An error occured */
                    else if (nx_status != NX_NOT_SUCCESSFUL) {
                        error_handler();
                    }

                    /* Change the link up check timer to have a longer period */
                    tx_status = tx_timer_change(&link_check_timer, NX_APP_LINK_CHECK_WHEN_UP_INTERVAL, NX_APP_LINK_CHECK_WHEN_UP_INTERVAL);
                    TX_CHECK(tx_status);
                }
            }

            /* The link is down */
            else if (nx_status == NX_NOT_SUCCESSFUL) {

                /* The link just went down */
                if (!linkdown) {

                    linkdown = true;

                    nx_status = nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_DISABLE, &actual_status);
                    NX_CHECK(nx_status);

                    /* Set the DHCP Client's remaining lease time to 0 seconds to trigger an immediate renewal request for a DHCP address. */
                    nx_status = nx_dhcp_client_update_time_remaining(&dhcp_client, 0);
                    NX_CHECK(nx_status);

                    /* Change the link up check timer to have a shorter period */
                    tx_status = tx_timer_change(&link_check_timer, NX_APP_LINK_CHECK_WHEN_DOWN_INTERVAL, NX_APP_LINK_CHECK_WHEN_DOWN_INTERVAL);
                    TX_CHECK(tx_status);
                }
            }

            /* An error occured */
            else {
                error_handler();
            }
        }

#if ENABLE_DHCP_RESTORE

        /* Periodically save the DHCP record to non-volatile memory in case of a reboot or link down */
        if (events & LINK_EVENT_DHCP_SAVE) {
            nx_status = dhcp_record_store(&dhcp_client);
            NX_CHECK(nx_status);
        }

#endif
    }
}


static void ip_address_change_notify_callback(NX_IP *ip_instance, void *ptr) {

    tx_status_t tx_status = TX_SUCCESS;
    nx_status_t nx_status = NX_SUCCESS;

    /* Get the new IP address */
    nx_status = nx_ip_address_get(&nx_ip_instance, &ip_address, &net_mask);
    NX_CHECK(nx_status);

    /* IP address has been assigned */
    if (ip_address != NULL_ADDRESS) {

        /* Notify the state machine */
        tx_status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED, TX_OR);
        TX_CHECK(tx_status);
    }

    /* IP address has been unassigned */
    else {

        /* Notify the state machine */
        tx_status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_NX_IP_ADDRESS_UNASSIGNED, TX_OR);
        TX_CHECK(tx_status);
    }
}


#if ENABLE_DHCP_RESTORE


static nx_status_t dhcp_record_set_stored_valid(bool valid) {

    nx_status_t status   = NX_SUCCESS;
    uint8_t     reg_data = valid;
    int         bytes_written;

    bytes_written = s_write_user_storage(USER_STORAGE_DHCP_VALID_ADDR, &reg_data, USER_STORAGE_DHCP_VALID_SIZE);
    if (bytes_written != USER_STORAGE_DHCP_VALID_SIZE) status = NX_STATUS_DHCP_WRITE_ERROR;

    return status;
}


static nx_status_t dhcp_record_get_stored_valid(bool *valid) {

    nx_status_t status = NX_SUCCESS;
    uint8_t     reg_data;
    int         bytes_read;

    bytes_read = s_read_user_storage(USER_STORAGE_DHCP_VALID_ADDR, &reg_data, USER_STORAGE_DHCP_VALID_SIZE);
    if ((bytes_read != USER_STORAGE_DHCP_VALID_SIZE) || (reg_data > true)) status = NX_STATUS_DHCP_READ_ERROR;

    if (status == NX_SUCCESS) *valid = reg_data;

    return status;
}


static nx_status_t dhcp_record_restore(NX_DHCP *client) {

    nx_status_t           status = NX_SUCCESS;
    NX_DHCP_CLIENT_RECORD record;
    bool                  valid;
    int                   bytes_read;

    /* Attempt to read the record valid flag */
    status = dhcp_record_get_stored_valid(&valid);
    if ((status != NX_SUCCESS) || !valid) return status;

    /* Attempt to read the record */
    bytes_read = s_read_user_storage(USER_STORAGE_DHCP_RECORD_ADDR, (uint8_t *) &record, USER_STORAGE_DHCP_RECORD_SIZE);
    if (bytes_read != USER_STORAGE_DHCP_RECORD_SIZE) status = NX_STATUS_DHCP_READ_ERROR;
    if (status != NX_SUCCESS) return status;

    /* Just return if the DHCP record wasn't started */
    if (record.nx_dhcp_state == NX_DHCP_STATE_NOT_STARTED) {
        return status;
    }

    /* Attempt to restore the record */
    status = nx_dhcp_client_restore_record(client, &record, 0); /* TODO: Set time elapsed based on RTC while asleep */
    if (status == NX_SUCCESS) {
        LOG_INFO("Restored DHCP record");
    } else {
        LOG_INFO("Failed to restore DHCP record, status = %d", status);
    }

    return status;
}


static nx_status_t dhcp_record_store(NX_DHCP *client) {

    nx_status_t           status = NX_SUCCESS;
    NX_DHCP_CLIENT_RECORD record;
    bool                  valid;
    int                   bytes_written;

    /* Attempt to get the record */
    status = nx_dhcp_client_get_record(client, &record);
    if (status == NX_SUCCESS) {

        /* Invalidate the old record */
        status = dhcp_record_set_stored_valid(false);

        /* Invalidate succeeded */
        if (status == NX_SUCCESS) {

            /* Attempt to write the record to non-volatile storage */
            bytes_written = s_write_user_storage(USER_STORAGE_DHCP_RECORD_ADDR, (uint8_t *) &record, USER_STORAGE_DHCP_RECORD_SIZE);
            valid         = bytes_written == USER_STORAGE_DHCP_RECORD_SIZE;

            /* If the write was successful set the valid flag */
            bytes_written = s_write_user_storage(USER_STORAGE_DHCP_VALID_ADDR, (uint8_t *) &valid, USER_STORAGE_DHCP_VALID_SIZE);
            if ((bytes_written != USER_STORAGE_DHCP_VALID_SIZE) || !valid) status = NX_STATUS_DHCP_WRITE_ERROR;
        }

        if (status == NX_SUCCESS) {
            LOG_INFO("Stored DHCP record");
        } else {
            LOG_INFO("Failed to store DHCP record, status = %d", status);
        }
    }

    /* If the record couldn't be found make sure its for a valid reason */
    else if ((status == NX_DHCP_NO_INTERFACES_STARTED) || (status == NX_DHCP_NO_INTERFACES_ENABLED)) {
        status = NX_SUCCESS;
    }

    return status;
}


#endif


void link_check_timer_callback(ULONG id) {
    tx_status_t status = tx_event_flags_set(&link_events_handle, LINK_EVENT_LINK_CHECK, TX_OR);
    TX_CHECK(status);
}


void dhcp_save_timer_callback(ULONG id) {
    tx_status_t status = tx_event_flags_set(&link_events_handle, LINK_EVENT_DHCP_SAVE, TX_OR);
    TX_CHECK(status);
}
