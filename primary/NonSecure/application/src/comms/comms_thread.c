/*
 * comms_thread.c
 *
 *  Created on: Aug 1, 2025
 *      Author: bens1
 */

#include "stdio.h"
#include "tx_api.h"
#include "nx_api.h"
#include "nx_stm32_eth_config.h"
#include "main.h"

#include "pb_encode.h"
#include "heartbeat.pb.h"

#include "tx_app.h"
#include "nx_app.h"
#include "utils.h"
#include "config.h"
#include "encodings.h"
#include "zenoh-pico.h"
#include "zenoh_cleanup.h"
#include "comms_thread.h"
#include "switch_thread.h"
#include "state_machine.h"


typedef enum {
    HEARTBEAT_NOT_STARTED,
    HEARTBEAT_CONSUMING,
    HEARTBEAT_PRODUCING,
    HEARTBEAT_MISSED,
} heartbeat_state_t;


uint8_t   comms_thread_stack[COMMS_THREAD_STACK_SIZE];
TX_THREAD comms_thread_handle;

uint8_t      zenoh_byte_pool_buffer[ZENOH_MEM_POOL_SIZE] __ALIGNED(32);
TX_BYTE_POOL zenoh_byte_pool;

/* Heartbeat variables */
static atomic_uint_fast32_t      heartbeat_consumer_timestamp = 0;                     /* The last time a heartbeat was received */
static _Atomic heartbeat_state_t heartbeat_consumer_state     = HEARTBEAT_NOT_STARTED; /* The state of the heartbeat consumer */
static uint8_t                   heartbeat_producer_buffer[HEARTBEAT_PB_H_MAX_SIZE];

zenoh_event_counters_t zenoh_events;

/* Publishers */
z_owned_publisher_t        stats_pub;
static z_owned_publisher_t heartbeat_pub;

/* Publisher options */
static const z_publisher_options_t heartbeat_pub_options = {
    .encoding           = NULL,
    .congestion_control = Z_CONGESTION_CONTROL_BLOCK, /* This is an important message so don't drop it */
    .priority           = Z_PRIORITY_DATA_HIGH,       /* Slightly higher priority than regular data */
    .is_express         = true,                       /* Disable batching so the message is sent immediately */
    .reliability        = Z_RELIABILITY_RELIABLE,
};


tx_status_t zenoh_connected(bool update_state_machine) {

    tx_status_t status = TX_SUCCESS;

    status = tx_event_flags_set(&state_machine_events_handle, ~STATE_MACHINE_ZENOH_DISCONNECTED, TX_AND);
    if (status != TX_SUCCESS) return status;
    status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_ZENOH_CONNECTED | ((update_state_machine) ? STATE_MACHINE_UPDATE : 0), TX_OR);

    zenoh_events.connections++;

    return status;
}

tx_status_t zenoh_disconnected(bool update_state_machine) {

    tx_status_t status = TX_SUCCESS;

    status = tx_event_flags_set(&state_machine_events_handle, ~STATE_MACHINE_ZENOH_CONNECTED, TX_AND);
    if (status != TX_SUCCESS) return status;
    status = tx_event_flags_set(&state_machine_events_handle, STATE_MACHINE_ZENOH_DISCONNECTED | ((update_state_machine) ? STATE_MACHINE_UPDATE : 0), TX_OR);

    return status;
}

/* Called when a message is received from the topic ZENOH_SUB_HEARTBEAT_KEYEXPR */
void heartbeat_sub_callback(z_loaned_sample_t *sample, void *ctx) {
    UNUSED(ctx);
    heartbeat_consumer_timestamp = tx_time_get_ms();
    if (heartbeat_consumer_state == HEARTBEAT_NOT_STARTED) {
        heartbeat_consumer_state = HEARTBEAT_CONSUMING;
    }
}


/* This thread manages the Zenoh Pico session initialisation, reconnection and heartbeats */
void comms_thread_entry(uint32_t initial_input) {

    tx_status_t tx_status             = TX_SUCCESS;
    _z_res_t    z_status              = Z_OK;
    bool        session_open          = false;
    uint32_t    restart_retry_counter = 0;
    uint32_t    failure_streak        = 0;
    bool        failure_streak_valid  = false;
    uint32_t    current_time          = tx_time_get_ms();

    Heartbeat    heartbeat = Heartbeat_init_zero;
    pb_ostream_t heartbeat_stream;

    z_owned_encoding_t heartbeat_encoding;

    memset(&zenoh_events, 0, sizeof(zenoh_event_counters_t));
    memset(zenoh_udp_multicast_groups_valid, 0, sizeof(zenoh_udp_multicast_groups_valid));

    while (1) {

        /* Reset variables */
        tx_status                = TX_SUCCESS;
        z_status                 = Z_OK;
        session_open             = false;
        heartbeat_consumer_state = HEARTBEAT_NOT_STARTED;

        /* Initialise the byte pool */
        tx_status = tx_byte_pool_create(&zenoh_byte_pool, "Zenoh Pico memory pool", zenoh_byte_pool_buffer, ZENOH_MEM_POOL_SIZE);
        if (tx_status != TX_SUCCESS) Error_Handler();

        /* Read and apply the config */
        z_owned_config_t config;
        z_status = z_config_default(&config);
        if (z_status < Z_OK) Error_Handler();
        z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, ZENOH_MODE);
        if (z_status < Z_OK) Error_Handler();
        if (strcmp(ZENOH_LOCATOR, "") != 0) {
            if (strcmp(ZENOH_MODE, Z_CONFIG_MODE_CLIENT) == 0) {
                z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, ZENOH_LOCATOR);
                if (z_status < Z_OK) Error_Handler();
            } else {
                z_status = zp_config_insert(z_loan_mut(config), Z_CONFIG_LISTEN_KEY, ZENOH_LOCATOR);
                if (z_status < Z_OK) Error_Handler();
            }
        }

        /* Create the options */
        z_open_options_t opts;
        z_open_options_default(&opts);

        /* Start a session */
        ns_log_write("Zenoh Pico: Attempting to open session\n");
        z_owned_session_t session;
        do {

            /* Attempt to open session */
            z_status = z_open(&session, z_move(config), &opts);

            /* Session open */
            if (z_status == Z_OK) {
                failure_streak       = 0;
                failure_streak_valid = false;
                break;
            }

            /* Try again */
            else if ((z_status == _Z_ERR_SCOUT_NO_RESULTS)                 /* Unable to find other Zenoh devices */
                     || (z_status == _Z_ERR_CONFIG_LOCATOR_SCHEMA_UNKNOWN) /* Can occur when the router is not configured with an incorrect transport or incorrect IP version (e.g. IPv6) */
            ) {
                ns_log_write("Zenoh Pico: Failed to open session, error code %i\n", z_status);
                failure_streak_valid = false;
                goto retry;
            }

            /* Try again, note these contribute to a failure streak because they indicate a remote failure */
            else if ((z_status == _Z_ERR_MESSAGE_DESERIALIZATION_FAILED) /* Random error TODO: find out why this happens */
                     || (z_status == _Z_ERR_MESSAGE_UNEXPECTED)          /* Message received while supposed to be shut down. This can occur when the server's heartbeat dies but the router is still running */
            ) {
                failure_streak++;
                failure_streak_valid = true;
                ns_log_write("Zenoh Pico: Failed to open session (%lu), error code %i\n", failure_streak, z_status);
                goto retry;
            }

            /* Unhandled error */
            else {
                Error_Handler();
            }

        } while (z_status < Z_OK);

        ns_log_write("Zenoh Pico: Session opened\n");

        /* Declare stats publisher */
        z_owned_keyexpr_t stats_pub_key;
        z_view_keyexpr_t  stats_pub_view_key;
        z_view_keyexpr_from_str(&stats_pub_view_key, ZENOH_PUB_STATS_KEYEXPR);
        z_status = z_declare_keyexpr(z_loan(session), &stats_pub_key, z_loan(stats_pub_view_key));
        if (z_status < Z_OK) Error_Handler();
        z_status = z_declare_publisher(z_loan(session), &stats_pub, z_loan(stats_pub_key), NULL);
        if (z_status < Z_OK) Error_Handler();

        /* Declare heartbeat publisher */
        z_owned_keyexpr_t heartbeat_pub_key;
        z_view_keyexpr_t  heartbeat_pub_view_key;
        z_view_keyexpr_from_str(&heartbeat_pub_view_key, ZENOH_PUB_HEARTBEAT_KEYEXPR);
        z_status = z_declare_keyexpr(z_loan(session), &heartbeat_pub_key, z_loan(heartbeat_pub_view_key));
        if (z_status < Z_OK) Error_Handler();
        z_status = z_declare_publisher(z_loan(session), &heartbeat_pub, z_loan(heartbeat_pub_key), &heartbeat_pub_options);
        if (z_status < Z_OK) Error_Handler();

        /* Declare heartbeat background subscriber */
        z_owned_keyexpr_t heartbeat_sub_key;
        z_view_keyexpr_t  heartbeat_sub_view_key;
        z_view_keyexpr_from_str(&heartbeat_sub_view_key, ZENOH_SUB_HEARTBEAT_KEYEXPR);
        z_status = z_declare_keyexpr(z_loan(session), &heartbeat_sub_key, z_loan(heartbeat_sub_view_key));
        if (z_status < Z_OK) Error_Handler();
        z_owned_closure_sample_t heartbeat_closure;
        z_closure(&heartbeat_closure, heartbeat_sub_callback, NULL, NULL);
        z_status = z_declare_background_subscriber(z_loan(session), z_loan(heartbeat_sub_key), z_move(heartbeat_closure), NULL);
        if (z_status < Z_OK) Error_Handler();

        /* Notify the state machine that we are connected and ready to communicate */
        tx_status = zenoh_connected(true);
        if (tx_status != TX_SUCCESS) Error_Handler();

        ns_log_write("Zenoh Pico: Entering main loop\n");

        /* Enter the main loop */
        session_open = !z_session_is_closed(z_loan(session));
        while (session_open) {

            current_time = tx_time_get_ms();

            /* Check if the server heartbeat has stopped.
             * If it has then record the error. Since the internal state
             * of the session is still intact (heartbeat miss is an
             * external error), then it is safe to close the session normally.
             */
            if ((heartbeat_consumer_state == HEARTBEAT_CONSUMING) &&
                (current_time > (heartbeat_consumer_timestamp + HEARTBEAT_MISS_TIMEOUT))) {
                heartbeat_consumer_state = HEARTBEAT_MISSED;
                zenoh_events.heartbeats_missed++;
                ns_log_write("Zenoh Pico: Heartbeat missed\n");
                goto close;
            }

            /* Create the heartbeat payload  */
            heartbeat_stream         = pb_ostream_from_buffer(heartbeat_producer_buffer, HEARTBEAT_PB_H_MAX_SIZE);
            heartbeat.status         = ServiceStatus_OK; /* TODO: Send actual status */
            heartbeat.uptime         = current_time;
            heartbeat.error_code     = 0;                /* TODO: Send error code */
            heartbeat.has_error_code = false;            /* TODO: Send error code */
            if (!pb_encode(&heartbeat_stream, Heartbeat_fields, &heartbeat)) Error_Handler();
            z_owned_bytes_t payload;
            z_status = z_bytes_from_static_buf(&payload, heartbeat_producer_buffer, heartbeat_stream.bytes_written);
            if (z_status < Z_OK) goto restart;

            /* Publish heartbeat message */
            z_publisher_put_options_t options;
            z_publisher_put_options_default(&options);
            z_status = z_encoding_from_str(&heartbeat_encoding, ENCODING_HEARTBEAT);
            if (z_status < Z_OK) Error_Handler();
            options.encoding = z_move(heartbeat_encoding);
            z_status         = z_publisher_put(z_loan(heartbeat_pub), z_move(payload), &options);
            if (z_status < Z_OK) goto restart;

            /* Sleep for HEARTBEAT_INTERVAL ms. If the STATE_MACHINE_ZENOH_DISCONNECTED flag
             * is set then immediately wake up and try to reconnect.
             */
            _Static_assert(HEARTBEAT_INTERVAL > 0, "Heartbeat interval should be > 0");
            uint32_t flags;
            tx_status = tx_event_flags_get(&state_machine_events_handle, STATE_MACHINE_ZENOH_DISCONNECTED, TX_OR, &flags, HEARTBEAT_INTERVAL);
            if (tx_status == TX_SUCCESS) {
                ns_log_write("Zenoh Pico: Received disconnect event\n");
                goto restart;
            } else if (tx_status != TX_NO_EVENTS) {
                Error_Handler();
            }
            session_open = !z_session_is_closed(z_loan(session));
        }

        ns_log_write("Zenoh Pico: Session closed\n");

    /* Close the session gracefully */
    close:

        zenoh_events.closures++;

        /* Clean-up */
        z_drop(z_move(session));

    /* An error occured while running */
    restart:

        zenoh_events.restarts++;

        /* Notify the state machine */
        tx_status = zenoh_disconnected(true);
        if (tx_status != TX_SUCCESS) Error_Handler();

    /* An error occured while connecting */
    retry:

        restart_retry_counter++;
        zenoh_events.reconnection_attempts = restart_retry_counter - zenoh_events.restarts;

        /* Clear the session to make sure other threads can't send messages */
        z_internal_session_null(&session);

        /* Delete all threads, mutexes and semaphores created by Zenoh Pico */
        zenoh_cleanup_tx();

        /* Leave all multicast groups and delete all sockets created by Zenoh Pico */
        zenoh_cleanup_nx();

        /* Delete the byte pool */
        tx_status = tx_byte_pool_delete(&zenoh_byte_pool);
        if (tx_status != TX_SUCCESS) Error_Handler();

        ns_log_write("Zenoh Pico: Terminated\n");

        /* Break after termination before restarting. If there are many back to back attempts then wait for any leases to expire */
        if (failure_streak && !(failure_streak % ZENOH_MAX_RETRIES_BEFORE_LONG_PAUSE) && failure_streak_valid) {
            z_sleep_ms(Z_TRANSPORT_LEASE);
        } else {
            z_sleep_ms(ZENOH_OPEN_SESSION_INTERVAL);
        }
    }
}
