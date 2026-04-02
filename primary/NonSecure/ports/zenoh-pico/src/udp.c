/*
 * zenoh_udp.c
 *
 *  Created on: Sep 23, 2025
 *      Author: bens1
 */

#include "stdbool.h"
#include "stddef.h"
#include "string.h"
#include "stdio.h"

#include "hal.h"

#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/codec/serial.h"
#include "zenoh-pico/system/common/serial.h"
#include "zenoh-pico/system/link/udp.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#include "app.h"
#include "zenoh_generic_config.h"
#include "nx_app.h"
#include "comms_thread.h"
#include "zenoh_cleanup.h"


/* TODO: Refactor using nx_status */


#if Z_FEATURE_LINK_UDP_UNICAST == 1 || Z_FEATURE_LINK_UDP_MULTICAST == 1

z_result_t _z_create_endpoint_udp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port) {

    _z_res_t status = Z_OK;

    /* Parse and check the validity of the port */
    uint32_t port = strtoul(s_port, NULL, 10);
    if (port > NX_MAX_PORT) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        return status;
    }

    /* Parse and check the validity of the IP(v4) address */
    uint32_t b1, b2, b3, b4;
    if (sscanf(s_address, "%lu.%lu.%lu.%lu", &b1, &b2, &b3, &b4) != 4) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        return status;
    }
    if (((b1 > UINT8_MAX) || (b2 > UINT8_MAX) || (b3 > UINT8_MAX) || (b4 > UINT8_MAX)) || ((b1 == 0) && (b2 == 0) && (b3 == 0) && (b4 == 0))) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        return status;
    }

    /* Assign the parsed values */
    ep->port       = port;
    ep->ip_address = IP_ADDRESS(b1, b2, b3, b4);

    return status;
}

void _z_free_endpoint_udp(_z_sys_net_endpoint_t *ep) {
    memset(ep, 0, sizeof(_z_sys_net_endpoint_t));
}

#endif

/* ---------------------------------------------------------------------------- */
/* Unicast */
/* ---------------------------------------------------------------------------- */

#if Z_FEATURE_LINK_UDP_UNICAST == 1

z_result_t _z_open_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {

    _z_res_t status = Z_OK;

    /* Store the timeout */
    sock->timeout = tout;

    /* Allocate memory for the socket */
    sock->udp_socket = z_malloc(sizeof(NX_UDP_SOCKET));
    if (sock->udp_socket == NULL) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        return status;
    }

    /* Create the socket */
    if (nx_udp_socket_create(
            &nx_ip_instance,
            sock->udp_socket,
            "UDP Server Socket",
            NX_IP_NORMAL,
            (Z_FEATURE_FRAGMENTATION == 1) ? NX_FRAGMENT_OKAY : NX_DONT_FRAGMENT,
            NX_IP_TIME_TO_LIVE,
            5) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        z_free(sock->udp_socket);
        return status;
    }

    /* Bind to port */
    if (nx_udp_socket_bind(sock->udp_socket, NX_ANY_PORT, sock->timeout) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        if (nx_udp_socket_delete(sock->udp_socket) != NX_SUCCESS) error_handler();
        z_free(sock->udp_socket);
        return status;
    }

    return status;
}

z_result_t _z_listen_udp_unicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout) {

    _z_res_t status = _Z_RES_OK;

    UNUSED(sock);
    UNUSED(rep);
    UNUSED(tout);

    /* TODO: To be implemented */
    status = _Z_ERR_GENERIC;
    _Z_ERROR_LOG(status);

    return status;
}


void _z_close_udp_unicast(_z_sys_net_socket_t *sock) {

    _z_res_t status = Z_OK;

    UNUSED(status);

    /* Unbind from port */
    if (nx_udp_socket_unbind(sock->udp_socket) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
    }

    /* Delete the socket */
    if (nx_udp_socket_delete(sock->udp_socket) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
    }

    /* Free the memory */
    z_free(sock->udp_socket);
}


size_t _z_read_exact_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {

    uint16_t n   = 0;
    uint8_t *pos = ptr;

    do {
        size_t rb = _z_read_udp_unicast(sock, pos, len - n);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n   = n + rb;
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t) n);
    } while (n != len);

    return n;
}


size_t _z_read_udp_unicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len) {

    _z_res_t    status      = Z_OK;
    uint32_t    bytes_read  = SIZE_MAX;
    NX_PACKET  *data_packet = NULL;
    nx_status_t nx_status   = NX_SUCCESS;

    UNUSED(status);

    /* Receive a packet */
    nx_status = nx_udp_socket_receive(sock.udp_socket, &data_packet, sock.timeout);
    if (nx_status != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        return bytes_read;
    }

    /* Extract the data from the packet */
    if (data_packet->nx_packet_length <= len) {
        nx_status = nx_packet_data_extract_offset(data_packet, 0, ptr, (ULONG) len, &bytes_read);
        if (nx_status != NX_SUCCESS) {
            status = _Z_ERR_GENERIC;
            _Z_ERROR_LOG(status);
            bytes_read = SIZE_MAX;

            /* Don't return yet because the packet needs to be released */
        }
    }

    /* Overflow */
    else {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        bytes_read = SIZE_MAX;
    }

    /* Release the packet */
    nx_status = nx_packet_release(data_packet);
    if (nx_status != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        bytes_read = SIZE_MAX;
    }

    return bytes_read;
}


size_t _z_send_udp_unicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t rep) {

    TX_INTERRUPT_SAVE_AREA

    _z_res_t   status      = Z_OK;
    uint32_t   bytes_sent  = SIZE_MAX;
    NX_PACKET *data_packet = NULL;

    UNUSED(status);

    /* Allocate a packet TODO: Use two packet pools */
    if (nx_packet_allocate(&nx_packet_pool, &data_packet, NX_UDP_PACKET, sock.timeout) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        return bytes_sent;
    }

    /* Append the message to send */
    if (nx_packet_data_append(data_packet, (uint8_t *) ptr, len, &nx_packet_pool, sock.timeout) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        if (nx_packet_release(data_packet) != NX_SUCCESS) error_handler();
        return bytes_sent;
    }

    /* Send the packet */
    if (nx_udp_socket_send(sock.udp_socket, data_packet, rep.ip_address, rep.port) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        if (nx_packet_release(data_packet) != NX_SUCCESS) error_handler();
        return bytes_sent;
    }

    /* Atomically increment 64-bit counter */
    TX_DISABLE
    zenoh_events.bytes_sent += len;
    TX_RESTORE

    /* Update other statistics */
    zenoh_events.packets_sent++;
    bytes_sent = len;

    return bytes_sent;
}

#endif

/* ---------------------------------------------------------------------------- */
/* Multicast */
/* ---------------------------------------------------------------------------- */

#if Z_FEATURE_LINK_UDP_MULTICAST == 1

z_result_t _z_open_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, _z_sys_net_endpoint_t *lep, uint32_t tout, const char *iface) {

    _z_res_t status = Z_OK;
    uint32_t subnet_mask;

    sock->timeout    = tout;
    sock->udp_socket = z_malloc(sizeof(NX_UDP_SOCKET));
    if (sock->udp_socket == NULL) return _Z_ERR_GENERIC;

    if (nx_udp_socket_create(&nx_ip_instance, sock->udp_socket, "UDP Multicast",
                             NX_IP_NORMAL, (Z_FEATURE_FRAGMENTATION == 1) ? NX_FRAGMENT_OKAY : NX_DONT_FRAGMENT,
                             NX_IP_TIME_TO_LIVE, 5) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        z_free(sock->udp_socket);
        return status;
    }

    if (nx_udp_socket_bind(sock->udp_socket, NX_ANY_PORT, sock->timeout) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        if (nx_udp_socket_delete(sock->udp_socket) != NX_SUCCESS) error_handler();
        z_free(sock->udp_socket);
        return status;
    }

    /* Store information about the endpoint */
    nx_ip_address_get(&nx_ip_instance, (ULONG *) &(lep->ip_address), (ULONG *) &subnet_mask);
    lep->port = sock->udp_socket->nx_udp_socket_port;

    return status;
}

z_result_t _z_listen_udp_multicast(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout, const char *iface, const char *join) {

    _z_res_t status = Z_OK;

    sock->timeout    = tout;
    sock->udp_socket = z_malloc(sizeof(NX_UDP_SOCKET));
    if (sock->udp_socket == NULL) return _Z_ERR_GENERIC;

    if (nx_udp_socket_create(&nx_ip_instance, sock->udp_socket, "UDP Multicast",
                             NX_IP_NORMAL, (Z_FEATURE_FRAGMENTATION == 1) ? NX_FRAGMENT_OKAY : NX_DONT_FRAGMENT,
                             NX_IP_TIME_TO_LIVE, 5) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        goto free;
    }

    if (nx_udp_socket_bind(sock->udp_socket, NX_ANY_PORT, sock->timeout) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        goto delete;
    }

    /* Join the multicast group */
    if (nx_ipv4_multicast_interface_join(&nx_ip_instance, rep.ip_address, PRIMARY_INTERFACE) != NX_SUCCESS) {
        status = _Z_ERR_GENERIC;
        goto unbind;
    }

    /* TODO: In Zenoh "join" can pipe-delimit multiple IP addresses */
    if (join != NULL) error_handler();

    /* Save the IP address of the group. TODO: Check if this is still needed */
    bool slot_found = false;
    for (uint_fast8_t i = 0; !slot_found && (i < NX_MAX_MULTICAST_GROUPS); i++) {
        if (!zenoh_udp_multicast_groups_valid[i]) continue;
        zenoh_udp_multicast_groups_list[i]  = rep.ip_address;
        zenoh_udp_multicast_groups_valid[i] = true;
        slot_found                          = true;
    }
    if (!slot_found) error_handler();

    return status;

unbind:
    if (nx_udp_socket_unbind(sock->udp_socket) != NX_SUCCESS) error_handler();
    delete : if (nx_udp_socket_delete(sock->udp_socket) != NX_SUCCESS) error_handler();
free:
    z_free(sock->udp_socket);

    return status;
}


void _z_close_udp_multicast(_z_sys_net_socket_t *sockrecv, _z_sys_net_socket_t *socksend, const _z_sys_net_endpoint_t rep, const _z_sys_net_endpoint_t lep) {

    _z_res_t status = Z_OK;

    UNUSED(lep);
    UNUSED(status);

    /* Close the receive socket */
    if (sockrecv != NULL) {

        /* Attempt to leave the multicast group */
        if (nx_ipv4_multicast_interface_leave(&nx_ip_instance, rep.ip_address, PRIMARY_INTERFACE) != NX_SUCCESS) {
            status = _Z_ERR_GENERIC;
            _Z_ERROR_LOG(status);
            error_handler();
        }

        /* Delete the IP address of the group. TODO: is this still necessary */
        bool slot_found = false;
        for (uint_fast8_t i = 0; !slot_found && (i < NX_MAX_MULTICAST_GROUPS); i++) {
            if (!(zenoh_udp_multicast_groups_valid[i] && (zenoh_udp_multicast_groups_list[i] == rep.ip_address))) continue;
            zenoh_udp_multicast_groups_valid[i] = false;
            slot_found                          = true;
        }
        if (!slot_found) Error_Handler();

        /* Close the socket */
        _z_close_udp_unicast(sockrecv);
    }

    /* Socket is NULL */
    else {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        error_handler();
    }

    /* Close the send socket */
    if ((socksend != NULL) && (socksend != sockrecv)) {

        /* Delete the IP address of the group. TODO: is this still necessary */
        bool slot_found = false;
        for (uint_fast8_t i = 0; !slot_found && (i < NX_MAX_MULTICAST_GROUPS); i++) {
            if (!(zenoh_udp_multicast_groups_valid[i] && (zenoh_udp_multicast_groups_list[i] == rep.ip_address))) continue;
            zenoh_udp_multicast_groups_valid[i] = false;
            slot_found                          = true;
        }
        if (!slot_found) error_handler();

        /* Close the socket */
        _z_close_udp_unicast(socksend);
    }

    /* Weird but acceptable case */
    else if (socksend == sockrecv) {
    }

    /* Socket is NULL */
    else {
        status = _Z_ERR_GENERIC;
        _Z_ERROR_LOG(status);
        error_handler();
    }
}


size_t _z_read_exact_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len,
                                   const _z_sys_net_endpoint_t lep, _z_slice_t *addr) {
    uint16_t n   = 0;
    uint8_t *pos = ptr;

    do {
        size_t rb = _z_read_udp_multicast(sock, pos, len - n, lep, addr);
        if ((rb == SIZE_MAX) || (rb == 0)) {
            n = rb;
            break;
        }

        n   = n + rb;
        pos = _z_ptr_u8_offset(pos, (ptrdiff_t) n);
    } while (n != len);

    return n;
}


size_t _z_read_udp_multicast(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t lep, _z_slice_t *ep) {

    _z_res_t    status      = Z_OK;
    nx_status_t nx_status   = NX_SUCCESS;
    uint32_t    bytes_read  = SIZE_MAX;
    NX_PACKET  *data_packet = NULL;
    UINT        source_port;
    ULONG       source_ip;

    UNUSED(status);

    /* Receive multiple packets and discard any received via the loopback interface.
     * This is to prevent us processing our own multicast messages */
    do {

        /* Receive a packet */
        nx_status = nx_udp_socket_receive(sock.udp_socket, &data_packet, sock.timeout);
        if (nx_status != NX_SUCCESS) {
            status = _Z_ERR_GENERIC;
            _Z_ERROR_LOG(status);
            return bytes_read;
        }

        /* Discard loopback */
        nx_status = nx_udp_source_extract(data_packet, &source_ip, &source_port);
        if (nx_status != NX_SUCCESS) error_handler();
        if (lep.port == source_port) {
            if (nx_packet_release(data_packet) != NX_SUCCESS) error_handler();
            continue;
        }

        /* Extract the data from the packet */
        if (data_packet->nx_packet_length <= len) {
            nx_status = nx_packet_data_extract_offset(data_packet, 0, ptr, (ULONG) len, &bytes_read);
            if (nx_status != NX_SUCCESS) {
                status = _Z_ERR_GENERIC;
                _Z_ERROR_LOG(status);
                if (nx_packet_release(data_packet) != NX_SUCCESS) error_handler();
                bytes_read = SIZE_MAX;
                return bytes_read;
            }
        }

        /* Overflow */
        else {
            status = _Z_ERR_GENERIC;
            _Z_ERROR_LOG(status);
            if (nx_packet_release(data_packet) != NX_SUCCESS) error_handler();
            bytes_read = SIZE_MAX;
            return bytes_read;
        }

        /* Release the packet */
        nx_status = nx_packet_release(data_packet);
        if (nx_status != NX_SUCCESS) {
            status = _Z_ERR_GENERIC;
            _Z_ERROR_LOG(status);
            bytes_read = SIZE_MAX;
            return bytes_read;
        }

        /* Store endpoint data: 4 bytes IPv4 address, 2 bytes port */
        if ((ep != NULL) && (ep->len >= 6)) {
            ep->len = 6;

            /* Cast away const to write to the underlying buffer */
            uint8_t *ep_buf = (uint8_t *) ep->start;

            /* Zenoh needs network byte order (big endian) */
            ep_buf[0] = (uint8_t) (source_ip >> 24);
            ep_buf[1] = (uint8_t) (source_ip >> 16);
            ep_buf[2] = (uint8_t) (source_ip >> 8);
            ep_buf[3] = (uint8_t) (source_ip >> 0);

            ep_buf[4] = (uint8_t) (source_port >> 8);
            ep_buf[5] = (uint8_t) (source_port >> 0);

            ep->_delete_context.deleter = NULL;
            ep->_delete_context.context = NULL;
        }

        return bytes_read;

    } while (1);
}


size_t _z_send_udp_multicast(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len, const _z_sys_net_endpoint_t rep) {
    return _z_send_udp_unicast(sock, ptr, len, rep);
}

#endif
