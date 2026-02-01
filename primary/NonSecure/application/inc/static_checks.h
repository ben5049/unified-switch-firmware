#include "stdint.h"

#include "tx_api.h"
#include "nxd_dhcp_client.h"

#include "secure_nsc.h"


#define CHECK_MEMBER_OFFSET(instance, base, member)                     \
    static_assert(offsetof(instance, member) == offsetof(base, member), \
                  #member " offset mismatch")

#define CHECK_MEMBER_SIZE(instance, base, member)                                   \
    static_assert(sizeof(((instance *) 0)->member) == sizeof(((base *) 0)->member), \
                  #member " size mismatch")

#define CHECK_MEMBER_COMPATIBILITY(instance, base, member) \
    CHECK_MEMBER_OFFSET(instance, base, member);           \
    CHECK_MEMBER_SIZE(instance, base, member)


#ifdef NX_DHCP_CLIENT_RESTORE_STATE
#define CHECK_DHCP_RECORD_MEMBERS()                                                                         \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_state);               \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_ip_address);          \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_network_mask);        \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_gateway_address);     \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_interface_index);     \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_timeout);             \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_server_ip);           \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_lease_remain_time);   \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_lease_time);          \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_renewal_time);        \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_rebind_time);         \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_renewal_remain_time); \
    CHECK_MEMBER_COMPATIBILITY(NX_DHCP_CLIENT_RECORD, s_dhcp_client_record_t, nx_dhcp_rebind_remain_time)
#else
#define CHECK_DHCP_RECORD_MEMBERS()
#endif


#define CHECK_BASIC_TYPES()                                                               \
    static_assert(sizeof(ULONG) == sizeof(uint32_t), "sizeof(ULONG) != sizeof(uint32_t)")
