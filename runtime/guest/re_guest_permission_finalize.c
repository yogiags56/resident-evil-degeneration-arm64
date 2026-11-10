#include "re_guest_permission_finalize.h"

#define RE_GUEST_MEM_VALID_FLAGS (RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC)

static REGUESTMEMRegion *find_exact_region(
    REGUESTMEMORY *memory, uint32_t base, uint64_t size) {
    for (size_t i = 0; i < RE_GUEST_MEMORY_MAX_REGIONS; ++i) {
        REGUESTMEMRegion *region = &memory->regions[i];
        if (region->active && region->base == base && region->size == size)
            return region;
    }
    return NULL;
}

REGUESTMEMPermissionFinalizeStatus re_guest_memory_finalize_permissions(
    REGUESTMEMORY *memory,
    const REGUESTMEMPERMISSIONREQUEST *requests,
    size_t count,
    size_t *failed_index) {
    if (failed_index) *failed_index = count;
    if (!memory || (!requests && count) ||
        count > RE_GUEST_MEMORY_MAX_REGIONS)
        return RE_GUEST_PERMISSION_FINALIZE_E_ARGUMENT;

    /* Preflight every request. No permission metadata changes in this loop. */
    for (size_t i = 0; i < count; ++i) {
        const REGUESTMEMPERMISSIONREQUEST *request = &requests[i];

        if (!request->size ||
            (request->final_permissions & ~RE_GUEST_MEM_VALID_FLAGS) ||
            !request->final_permissions) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_PERMISSION_FINALIZE_E_ARGUMENT;
        }
        if (request->size > 0x100000000ULL - (uint64_t)request->guest_base) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_PERMISSION_FINALIZE_E_RANGE;
        }
        if ((request->final_permissions & RE_GUEST_MEM_WRITE) &&
            (request->final_permissions & RE_GUEST_MEM_EXEC)) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_PERMISSION_FINALIZE_E_WX;
        }
        if (!find_exact_region(memory, request->guest_base, request->size)) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_PERMISSION_FINALIZE_E_NOT_FOUND;
        }
        for (size_t j = 0; j < i; ++j) {
            if (requests[j].guest_base == request->guest_base &&
                requests[j].size == request->size) {
                if (failed_index) *failed_index = i;
                return RE_GUEST_PERMISSION_FINALIZE_E_DUPLICATE;
            }
        }
    }

    /* Commit: exact regions are known to exist; assignments cannot fail. */
    for (size_t i = 0; i < count; ++i) {
        REGUESTMEMRegion *region = find_exact_region(
            memory, requests[i].guest_base, requests[i].size);
        region->permissions = requests[i].final_permissions;
    }
    return RE_GUEST_PERMISSION_FINALIZE_OK;
}
