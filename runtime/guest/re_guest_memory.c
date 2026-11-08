#include "re_guest_memory.h"
#include <string.h>

void re_guest_memory_init(REGUESTMEMORY *m) {
    if (m) memset(m, 0, sizeof(*m));
}

REGUESTMEMStatus re_guest_memory_map(REGUESTMEMORY *m, uint32_t base,
    uint64_t size, void *host, uint32_t permissions) {
    uint64_t end;
    if (!m || !host || !size ||
        (permissions & ~(RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC)) ||
        !permissions) return RE_GUEST_MEM_E_ARGUMENT;
    end=(uint64_t)base+size;
    if (end>0x100000000ULL) return RE_GUEST_MEM_E_RANGE;
    for (size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
        REGUESTMEMRegion *r=&m->regions[i];
        uint64_t rend;
        if (!r->active) continue;
        rend=(uint64_t)r->base+r->size;
        if ((uint64_t)base<rend && (uint64_t)r->base<end)
            return RE_GUEST_MEM_E_OVERLAP;
    }
    for (size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
        REGUESTMEMRegion *r=&m->regions[i];
        if (!r->active) {
            r->base=base; r->size=size; r->host=(uint8_t *)host;
            r->permissions=permissions; r->active=1u; m->count++;
            return RE_GUEST_MEM_OK;
        }
    }
    return RE_GUEST_MEM_E_CAPACITY;
}

REGUESTMEMStatus re_guest_memory_unmap(REGUESTMEMORY *m, uint32_t base) {
    if (!m) return RE_GUEST_MEM_E_ARGUMENT;
    for (size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
        REGUESTMEMRegion *r=&m->regions[i];
        if (r->active && r->base==base) {
            memset(r,0,sizeof(*r)); if(m->count)m->count--;
            return RE_GUEST_MEM_OK;
        }
    }
    return RE_GUEST_MEM_E_NOT_FOUND;
}

REGUESTMEMStatus re_guest_memory_resolve(const REGUESTMEMORY *m,
    uint32_t address, size_t length, uint32_t required, void **out) {
    uint64_t end=(uint64_t)address+(uint64_t)length;
    if (out) *out=NULL;
    if (!m || !out || !length ||
        (required & ~(RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC)))
        return RE_GUEST_MEM_E_ARGUMENT;
    if (end>0x100000000ULL) return RE_GUEST_MEM_E_RANGE;
    for (size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
        const REGUESTMEMRegion *r=&m->regions[i];
        uint64_t rend=(uint64_t)r->base+r->size;
        if (!r->active || address<r->base || end>rend) continue;
        if ((r->permissions&required)!=required) return RE_GUEST_MEM_E_PERMISSION;
        *out=r->host+(size_t)(address-r->base);
        return RE_GUEST_MEM_OK;
    }
    return RE_GUEST_MEM_E_NOT_FOUND;
}

size_t re_guest_memory_region_count(const REGUESTMEMORY *m) {
    return m?m->count:0u;
}
