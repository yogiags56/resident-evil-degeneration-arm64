#ifndef RE_GUEST_MEMORY_VIEW_H
#define RE_GUEST_MEMORY_VIEW_H
#include "re_guest_cpu.h"
#include "re_guest_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Transitional compatibility adapter for the legacy contiguous CPU API.
 * The returned view borrows the region buffer and is invalidated by unmap,
 * remap, or owner deallocation. This is NOT a multi-region memory bus and
 * must not be used where per-access permissions need enforcement.
 */
REGUESTMEMStatus re_guest_memory_region_view(
    const REGUESTMEMORY *memory, uint32_t guest_base,
    uint32_t required_permissions, REGUESTMEM *out_view);

#ifdef __cplusplus
}
#endif
#endif
