#include "re_guest_memory_view.h"
#include <stdint.h>

REGUESTMEMStatus re_guest_memory_region_view(
    const REGUESTMEMORY *m, uint32_t base,
    uint32_t required, REGUESTMEM *out) {
    if (out) {
        out->bytes=NULL;
        out->size=0;
        out->guest_base=0;
    }
    if (!m || !out ||
        (required & ~(RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC)))
        return RE_GUEST_MEM_E_ARGUMENT;
    for (size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
        const REGUESTMEMRegion *r=&m->regions[i];
        if (!r->active || r->base!=base) continue;
        if ((r->permissions&required)!=required)
            return RE_GUEST_MEM_E_PERMISSION;
        if (r->size>(uint64_t)SIZE_MAX)
            return RE_GUEST_MEM_E_RANGE;
        out->bytes=r->host;
        out->size=(size_t)r->size;
        out->guest_base=r->base;
        return RE_GUEST_MEM_OK;
    }
    return RE_GUEST_MEM_E_NOT_FOUND;
}
