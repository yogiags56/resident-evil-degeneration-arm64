#ifndef RE_GUEST_PERMISSION_FINALIZE_H
#define RE_GUEST_PERMISSION_FINALIZE_H

#include "re_guest_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t guest_base;
    uint64_t size;
    uint32_t final_permissions;
} REGUESTMEMPERMISSIONREQUEST;

typedef enum {
    RE_GUEST_PERMISSION_FINALIZE_OK = 0,
    RE_GUEST_PERMISSION_FINALIZE_E_ARGUMENT = -300,
    RE_GUEST_PERMISSION_FINALIZE_E_RANGE = -301,
    RE_GUEST_PERMISSION_FINALIZE_E_NOT_FOUND = -302,
    RE_GUEST_PERMISSION_FINALIZE_E_WX = -303,
    RE_GUEST_PERMISSION_FINALIZE_E_DUPLICATE = -304
} REGUESTMEMPermissionFinalizeStatus;

/*
 * Atomically finalize software guest permissions for exact existing mappings.
 * W+X is rejected. This updates guest permission metadata only; it does not
 * invoke mprotect, alter host page permissions, or execute guest code.
 */
REGUESTMEMPermissionFinalizeStatus re_guest_memory_finalize_permissions(
    REGUESTMEMORY *memory,
    const REGUESTMEMPERMISSIONREQUEST *requests,
    size_t count,
    size_t *failed_index);

#ifdef __cplusplus
}
#endif
#endif
