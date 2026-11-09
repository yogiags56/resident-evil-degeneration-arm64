#include "re_guest_map_transaction.h"

REGUESTMEMMAPTXStatus re_guest_memory_map_transaction(
    REGUESTMEMORY *memory,
    const REGUESTMEMMAPREQUEST *requests,
    size_t count,
    size_t *failed_index,
    REGUESTMEMStatus *map_status) {
    size_t mapped = 0;

    if (failed_index) *failed_index = count;
    if (map_status) *map_status = RE_GUEST_MEM_OK;
    if (!memory || (!requests && count) || count > RE_GUEST_MEMORY_MAX_REGIONS) {
        if (map_status) *map_status = RE_GUEST_MEM_E_ARGUMENT;
        return RE_GUEST_MAP_TX_E_ARGUMENT;
    }

    for (size_t i = 0; i < count; ++i) {
        REGUESTMEMStatus status = re_guest_memory_map_ex(
            memory,
            requests[i].guest_base,
            requests[i].size,
            requests[i].host_buffer,
            requests[i].permissions,
            requests[i].memory_type);

        if (status != RE_GUEST_MEM_OK) {
            if (failed_index) *failed_index = i;
            if (map_status) *map_status = status;

            while (mapped > 0) {
                --mapped;
                if (re_guest_memory_unmap(memory, requests[mapped].guest_base)
                    != RE_GUEST_MEM_OK) {
                    return RE_GUEST_MAP_TX_E_ROLLBACK;
                }
            }
            return RE_GUEST_MAP_TX_E_MAP;
        }
        ++mapped;
    }
    return RE_GUEST_MAP_TX_OK;
}
