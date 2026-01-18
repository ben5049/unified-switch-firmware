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
#include "main.h"

#include "nx_app.h"
#include "config.h"
#include "utils.h"
#include "tx_app.h"
#include "state_machine.h"
#include "secure_nsc.h"


#define NULL_ADDRESS 0


uint32_t ip_address;
uint32_t net_mask;

TX_THREAD nx_link_thread_handle;
uint8_t   nx_link_thread_stack[NX_LINK_THREAD_STACK_SIZE];


static void ip_address_change_notify_callback(NX_IP *ip_instance, void *ptr) {

    /* Get the new IP address */
    if (nx_ip_address_get(&nx_ip_instance, &ip_address, &net_mask) != NX_SUCCESS) {
        Error_Handler();
    }

    /* IP address has been assigned */
    if (ip_address != NULL_ADDRESS) {

        /* Notify the state machine */
        if (tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED | STATE_MACHINE_UPDATE, TX_OR) != TX_SUCCESS) {
            Error_Handler();
        }
    }

    /* IP address has been unassigned */
    else {

        /* Notify the state machine */
        if (tx_event_flags_set(&state_machine_events_handle, ~STATE_MACHINE_NX_IP_ADDRESS_ASSIGNED, TX_AND) != TX_SUCCESS) {
            Error_Handler();
        }
        if (tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_UPDATE, TX_OR) != TX_SUCCESS) {
            Error_Handler();
        }
    }
}


/* TODO: Remember the dynamically assigned IP address (NX_DHCP_CLIENT_RESTORE_STATE) */
/* TODO: Respond to PHY linkup events or poll faster when the link is down */

/* This thread monitors the link state */
void nx_link_thread_entry(uint32_t thread_input) {

    uint32_t    actual_status = 0;
    nx_status_t nx_status     = NX_SUCCESS;
    tx_status_t tx_status     = TX_SUCCESS;
    bool        linkdown      = true;
    uint32_t    current_time;

    /* Register the IP address change callback */
    nx_status = nx_ip_address_change_notify(&nx_ip_instance, ip_address_change_notify_callback, NULL);
    if (nx_status != NX_SUCCESS) Error_Handler();

    /* Start DHCP */
    nx_status = nx_dhcp_start(&dhcp_client);
    if (nx_status != NX_SUCCESS) Error_Handler();

    /* FIXME: Move to after link up? */
#if ENABLE_DHCP_RESTORE == true
    /* Attempt to load and restore the DHCP record */
    NX_DHCP_CLIENT_RECORD dhcp_record;
    uint32_t              dhcp_record_next_save_time = 0;
    s_load_dhcp_client_record(&dhcp_record);
    if (dhcp_record.nx_dhcp_state != NX_DHCP_STATE_NOT_STARTED) {
        nx_status = nx_dhcp_client_restore_record(&dhcp_client, &dhcp_record, 0); /* TODO: Set time elapsed based on RTC while asleep */
        if (nx_status != NX_SUCCESS) Error_Handler();
    }
#endif

    while (1) {

        current_time = tx_time_get_ms();

        /* Send request to check if the switch is up and running */
        nx_status = nx_ip_interface_status_check(&nx_ip_instance, PRIMARY_INTERFACE, NX_IP_LINK_ENABLED, &actual_status, 10);

        /* The link is up */
        if (nx_status == NX_SUCCESS) {

            /* The link just went up */
            if (linkdown) {

                /* Send request to enable our MAC */
                nx_status = nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_ENABLE, &actual_status);
                if ((nx_status != NX_SUCCESS) && (nx_status != NX_ALREADY_ENABLED)) Error_Handler();

                /* Notify the state machine that the link is up */
                tx_status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_NX_LINK_UP | STATE_MACHINE_UPDATE, TX_OR);
                if (tx_status != TX_SUCCESS) Error_Handler();

                /* Send request to check if an address is resolved */
                nx_status = nx_ip_interface_status_check(&nx_ip_instance, PRIMARY_INTERFACE, NX_IP_ADDRESS_RESOLVED, &actual_status, 10);

                /* If we have an IP address then restart DHCP to get a new IP address for the new network */
                if (nx_status == NX_SUCCESS) {

                    /* Stop DHCP */
                    nx_status = nx_dhcp_stop(&dhcp_client);
                    if (nx_status != NX_SUCCESS) Error_Handler();

                    /* Re-initialize DHCP */
                    nx_status = nx_dhcp_reinitialize(&dhcp_client);
                    if (nx_status != NX_SUCCESS) Error_Handler();

                    /* Start DHCP */
                    nx_status = nx_dhcp_start(&dhcp_client);
                    if (nx_status != NX_SUCCESS) Error_Handler();
                }

                /* An error occured */
                else if (nx_status != NX_NOT_SUCCESSFUL) {
                    Error_Handler();
                }

                linkdown = false;
            }

        }

        /* The link is down */
        else if (nx_status == NX_NOT_SUCCESSFUL) {

            /* The link just went down */
            if (!linkdown) {
                nx_status = nx_ip_driver_direct_command(&nx_ip_instance, NX_LINK_DISABLE, &actual_status);
                if (nx_status != NX_SUCCESS) Error_Handler();

                /* Set the DHCP Client's remaining lease time to 0 seconds to trigger an immediate renewal request for a DHCP address. */
                nx_status = nx_dhcp_client_update_time_remaining(&dhcp_client, 0);
                if (nx_status != NX_SUCCESS) Error_Handler();

                linkdown = true;
            }

        }

        /* An error occured */
        else {
            Error_Handler();
        }

#if ENABLE_DHCP_RESTORE == true
        /* Periodically save the DHCP record to non-volatile memory in case of a reboot */
        if (dhcp_record_next_save_time <= current_time) {
            dhcp_record_next_save_time = current_time + DHCP_RECORD_SAVE_INTERVAL;
            nx_status                  = nx_dhcp_client_get_record(&dhcp_client, &dhcp_record);
            if (nx_status == NX_SUCCESS) {
                s_save_dhcp_client_record(&dhcp_record);
            } else if ((nx_status != NX_DHCP_NO_INTERFACES_STARTED) && (nx_status != NX_DHCP_NO_INTERFACES_ENABLED)) {
                Error_Handler();
            }
        }
#endif

        /* Delay differently based on link state. TODO: Use an event from the PHY thread to wake up faster */
        if (linkdown) {
            tx_thread_sleep_ms(NX_APP_CABLE_CONNECTION_CHECK_UP_PERIOD);
        } else {
            tx_thread_sleep_ms(NX_APP_CABLE_CONNECTION_CHECK_DOWN_PERIOD);
        }
    }
}
