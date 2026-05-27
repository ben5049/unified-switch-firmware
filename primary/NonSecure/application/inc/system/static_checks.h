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

#define CHECK_BASIC_TYPES()                                                               \
    static_assert(sizeof(ULONG) == sizeof(uint32_t), "sizeof(ULONG) != sizeof(uint32_t)")
