/*
 * nx_link_thread.c
 *
 *  Created on: Aug 12, 2025
 *      Author: bens1
 *
 */

#include "stdint.h"
#include "stdbool.h"
#include "tx_api.h"
#include "nx_api.h"

#include "app.h"
#include "nx_app.h"
#include "config.h"
#include "utils.h"
#include "tx_app.h"
#include "state_machine.h"
#include "secure_nsc.h"


#define NULL_ADDRESS 0


/* Private function prototypes */
static void ip_address_change_notify_callback(NX_IP *ip_instance, void *ptr);
#if ENABLE_DHCP_RESTORE
static nx_status_t restore_dhcp_record(NX_DHCP *client);
static nx_status_t store_dhcp_record(NX_DHCP *client);
#endif

uint32_t ip_address;
uint32_t net_mask;

TX_THREAD nx_link_thread_handle;
uint8_t   nx_link_thread_stack[NX_LINK_THREAD_STACK_SIZE];


/* This thread monitors the link state */
void nx_link_thread_entry(uint32_t thread_input) {

    uint32_t    actual_status = 0;
    nx_status_t nx_status     = NX_SUCCESS;
    tx_status_t tx_status     = TX_SUCCESS;
    bool        linkdown      = true;
    uint32_t    current_time;
#if ENABLE_DHCP_RESTORE
    uint32_t dhcp_record_next_save_time = 0;
#endif

    /* Register the IP address change callback */
    nx_status = nx_ip_address_change_notify(&nx_ip_instance, ip_address_change_notify_callback, NULL);
    if (nx_status != NX_SUCCESS) error_handler();

    /* Start DHCP */
    nx_status = nx_dhcp_start(&dhcp_client);
    if (nx_status != NX_SUCCESS) error_handler();

    /* Attempt to load and restore the DHCP record */
#if ENABLE_DHCP_RESTORE
    nx_status = restore_dhcp_record(&dhcp_client);
    if (nx_status != NX_SUCCESS) error_handler();
#endif

    while (1) {

        current_time = tx_time_get_ms();

        /* Send request to check if the switch is up and running */
        nx_status = nx_ip_interface_status_check(&nx_ip_instance, PRIMARY_INTERFACE, NX_IP_LINK_ENABLED, &actual_status, 0);

        /* The link is up */
        if (nx_status == NX_SUCCESS) {

            /* The link just went up */
            if (linkdown) {

                /* Send request to enable our MAC */
                nx_status = nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_ENABLE, (ULONG *) &actual_status);
                if ((nx_status != NX_SUCCESS) && (nx_status != NX_ALREADY_ENABLED)) error_handler();

                /* Notify the state machine that the link is up */
                tx_status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_NX_LINK_UP | STATE_MACHINE_UPDATE, TX_OR);
                if (tx_status != TX_SUCCESS) error_handler();

                /* Send request to check if an address is resolved */
                nx_status = nx_ip_interface_status_check(&nx_ip_instance, PRIMARY_INTERFACE, NX_IP_ADDRESS_RESOLVED, &actual_status, 0);

                /* If we have an IP address then restart DHCP to get a new IP address for the new network */
                if (nx_status == NX_SUCCESS) {

                    /* Stop DHCP */
                    nx_status = nx_dhcp_stop(&dhcp_client);
                    if (nx_status != NX_SUCCESS) error_handler();

                    /* Re-initialize DHCP */
                    nx_status = nx_dhcp_reinitialize(&dhcp_client);
                    if (nx_status != NX_SUCCESS) error_handler();

                    /* Start DHCP */
                    nx_status = nx_dhcp_start(&dhcp_client);
                    if (nx_status != NX_SUCCESS) error_handler();
                }

                /* An error occured */
                else if (nx_status != NX_NOT_SUCCESSFUL) {
                    error_handler();
                }

                linkdown = false;
            }
        }

        /* The link is down */
        else if (nx_status == NX_NOT_SUCCESSFUL) {

            /* The link just went down */
            if (!linkdown) {
                nx_status = nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_DISABLE, &actual_status);
                if (nx_status != NX_SUCCESS) error_handler();

                /* Set the DHCP Client's remaining lease time to 0 seconds to trigger an immediate renewal request for a DHCP address. */
                nx_status = nx_dhcp_client_update_time_remaining(&dhcp_client, 0);
                if (nx_status != NX_SUCCESS) error_handler();

                linkdown = true;
            }
        }

        /* An error occured */
        else {
            error_handler();
        }

        /* Periodically save the DHCP record to non-volatile memory in case of a reboot */
#if ENABLE_DHCP_RESTORE
        if (dhcp_record_next_save_time <= current_time) {
            dhcp_record_next_save_time = current_time + DHCP_RECORD_SAVE_INTERVAL;
            nx_status                  = store_dhcp_record(&dhcp_client);
            if (nx_status != NX_SUCCESS) error_handler();
        }
#endif

        /* Delay differently based on link state */
        // TODO: Use an event from the PHY thread to wake up faster
        if (linkdown) {
            tx_thread_sleep_ms(NX_APP_CABLE_CONNECTION_CHECK_UP_PERIOD);
        } else {
            tx_thread_sleep_ms(NX_APP_CABLE_CONNECTION_CHECK_DOWN_PERIOD);
        }
    }
}


static void ip_address_change_notify_callback(NX_IP *ip_instance, void *ptr) {

    /* Get the new IP address */
    if (nx_ip_address_get(&nx_ip_instance, &ip_address, &net_mask) != NX_SUCCESS) {
        error_handler();
    }

    /* IP address has been assigned */
    if (ip_address != NULL_ADDRESS) {

        /* Notify the state machine */
        if (tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED | STATE_MACHINE_UPDATE, TX_OR) != TX_SUCCESS) {
            error_handler();
        }
    }

    /* IP address has been unassigned */
    else {

        /* Notify the state machine */
        if (tx_event_flags_set(&state_machine_events_handle, ~STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED, TX_AND) != TX_SUCCESS) {
            error_handler();
        }
        if (tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_UPDATE, TX_OR) != TX_SUCCESS) {
            error_handler();
        }
    }
}


#if ENABLE_DHCP_RESTORE


static nx_status_t restore_dhcp_record(NX_DHCP *client) {

    nx_status_t           status = NX_SUCCESS;
    NX_DHCP_CLIENT_RECORD record;
    bool                  valid = false;
    int                   bytes_read;

    /* Attempt to read the record valid flag */
    bytes_read = s_read_user_storage(USER_STORAGE_DHCP_VALID_ADDR, (uint8_t *) &valid, USER_STORAGE_DHCP_VALID_SIZE);
    if (bytes_read != USER_STORAGE_DHCP_VALID_SIZE) status = NX_STATUS_DHCP_READ_ERROR;
    if ((status != NX_STATUS_SUCCESS) || !valid) return status;

    /* Attempt to read the record */
    bytes_read = s_read_user_storage(USER_STORAGE_DHCP_RECORD_ADDR, (uint8_t *) &record, USER_STORAGE_DHCP_RECORD_SIZE);
    if (bytes_read != USER_STORAGE_DHCP_RECORD_SIZE) status = NX_STATUS_DHCP_READ_ERROR;
    if (status != NX_STATUS_SUCCESS) return status;

    /* Just return if the DHCP record wasn't started */
    if (record.nx_dhcp_state == NX_DHCP_STATE_NOT_STARTED) {
        return status;
    }

    /* Attempt to restore the record */
    status = nx_dhcp_client_restore_record(client, &record, 0); /* TODO: Set time elapsed based on RTC while asleep */
    if (status == NX_STATUS_SUCCESS) {
        LOG_INFO("Successfully restored DHCP record");
    } else {
        LOG_INFO("Failed to restore DHCP record, status = %d", status);
    }

    return status;
}


static nx_status_t store_dhcp_record(NX_DHCP *client) {

    nx_status_t           status = NX_SUCCESS;
    NX_DHCP_CLIENT_RECORD record;
    bool                  valid;
    int                   bytes_written;

    /* Attempt to get the record */
    status = nx_dhcp_client_get_record(client, &record);
    if (status == NX_SUCCESS) {

        /* Attempt to write the record to non-volatile storage */
        bytes_written = s_write_user_storage(USER_STORAGE_DHCP_RECORD_ADDR, (uint8_t *) &record, USER_STORAGE_DHCP_RECORD_SIZE);
        valid         = bytes_written == USER_STORAGE_DHCP_RECORD_SIZE;

        /* If the write was successful set the valid flag */
        bytes_written = s_write_user_storage(USER_STORAGE_DHCP_VALID_ADDR, (uint8_t *) &valid, USER_STORAGE_DHCP_VALID_SIZE);
        if ((bytes_written != USER_STORAGE_DHCP_VALID_SIZE) || !valid) status = NX_STATUS_DHCP_WRITE_ERROR;

        if (status == NX_STATUS_SUCCESS) {
            LOG_INFO("Successfully stored DHCP record");
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
