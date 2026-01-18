/*
 * secure_nsc.c
 *
 *  Created on: Oct 5, 2025
 *      Author: bens1
 */

#include "secure_nsc.h"
#include "metadata.h"
#include "error.h"
#include "logging.h"
#include "boot_config.h"


#if DISABLE_S_SYSTICK_IN_NS == true
#define START_NSC HAL_ResumeTick()
#define END_NSC   HAL_SuspendTick()
#else
#define START_NSC
#define END_NSC
#endif


void s_save_dhcp_client_record(const NX_DHCP_CLIENT_RECORD *record) {
    memcpy(&hmeta.metadata.dhcp_record, record, sizeof(NX_DHCP_CLIENT_RECORD));
    hmeta.new_metadata = true;
}


void s_load_dhcp_client_record(NX_DHCP_CLIENT_RECORD *record) {
    if (hmeta.first_boot) {
        record->nx_dhcp_state = NX_DHCP_STATE_NOT_STARTED;
    } else {
        memcpy(record, &hmeta.metadata.dhcp_record, sizeof(NX_DHCP_CLIENT_RECORD));
    }
}


/* Call this function from a thread in ThreadX at 1Hz to do background work in the secure world */
void s_background_task(void) {

    metadata_status_t status = META_OK;

    if (hmeta.new_metadata) {
        status = META_dump_metadata(&hmeta);
        CHECK_STATUS_META(status);
    }

    if (hmeta.new_counters) {
        status = META_dump_counters(&hmeta);
        CHECK_STATUS_META(status);
    }
}


void s_log_vwrite(const char *format, va_list args) {

#if ENABLE_UART_LOGGING == true
    printf("NS %10lu: ", HAL_GetTick());
#endif

    log_status_t status = LOGGING_OK;

    status = log_vwrite(&hlog, format, args);
    CHECK_STATUS_LOG(status);
}
