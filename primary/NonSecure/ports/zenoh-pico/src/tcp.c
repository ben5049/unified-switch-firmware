/*
 * zenoh_tcp.c
 *
 *  Created on: Sep 23, 2025
 *      Author: bens1
 */

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "hal.h"
#include "zenoh-pico/collections/slice.h"
#include "zenoh-pico/collections/string.h"
#include "zenoh-pico/protocol/codec/serial.h"
#include "zenoh-pico/system/common/serial.h"
#include "zenoh-pico/system/link/tcp.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/checksum.h"
#include "zenoh-pico/utils/encoding.h"
#include "zenoh-pico/utils/logging.h"
#include "zenoh-pico/utils/pointers.h"

#include "zenoh_generic_config.h"


#if Z_FEATURE_LINK_TCP == 1
#error "Z_FEATURE_LINK_TCP is not supported"

z_result_t _z_create_endpoint_tcp(_z_sys_net_endpoint_t *ep, const char *s_address, const char *s_port);
void       _z_free_endpoint_tcp(_z_sys_net_endpoint_t *ep);

z_result_t _z_open_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep, uint32_t tout);
z_result_t _z_listen_tcp(_z_sys_net_socket_t *sock, const _z_sys_net_endpoint_t rep);
void       _z_close_tcp(_z_sys_net_socket_t *sock);
size_t     _z_read_exact_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t     _z_read_tcp(const _z_sys_net_socket_t sock, uint8_t *ptr, size_t len);
size_t     _z_send_tcp(const _z_sys_net_socket_t sock, const uint8_t *ptr, size_t len);
#endif