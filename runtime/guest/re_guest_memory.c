#include "re_guest_memory.h"
#include <string.h>
#define RE_GUEST_ADDRESS_SPACE_SIZE 0x100000000ULL
#define RE_GUEST_MEM_VALID_FLAGS (RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC)

void re_guest_memory_init(REGUESTMEMORY *m) {
 if(m) memset(m,0,sizeof(*m));
}

REGUESTMEMStatus re_guest_memory_map_ex(REGUESTMEMORY *m,uint32_t base,
 uint64_t size,void *host,uint32_t perms,REGUESTMEMType type) {
 uint64_t end;
 if(!m||!host||!size||(perms&~RE_GUEST_MEM_VALID_FLAGS)||!perms||
    type<RE_GUEST_MEM_TYPE_UNKNOWN||type>RE_GUEST_MEM_TYPE_STRONGLY_ORDERED)
  return RE_GUEST_MEM_E_ARGUMENT;
 if(size>(uint64_t)SIZE_MAX) return RE_GUEST_MEM_E_RANGE;
 if(size>RE_GUEST_ADDRESS_SPACE_SIZE-(uint64_t)base) return RE_GUEST_MEM_E_RANGE;
 end=(uint64_t)base+size;
 for(size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
  REGUESTMEMRegion *r=&m->regions[i];
  uint64_t rend;
  if(!r->active) continue;
  rend=(uint64_t)r->base+r->size;
  if((uint64_t)base<rend&&(uint64_t)r->base<end) return RE_GUEST_MEM_E_OVERLAP;
 }
 for(size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
  REGUESTMEMRegion *r=&m->regions[i];
  if(!r->active) {
   r->base=base;r->size=size;r->host=(uint8_t *)host;
   r->permissions=perms;r->active=1u;r->memory_type=type;
   m->count++;
   return RE_GUEST_MEM_OK;
  }
 }
 return RE_GUEST_MEM_E_CAPACITY;
}

/* Preserve the old API while making its missing memory type explicit. */
REGUESTMEMStatus re_guest_memory_map(REGUESTMEMORY *m,uint32_t base,
 uint64_t size,void *host,uint32_t perms) {
 return re_guest_memory_map_ex(m,base,size,host,perms,RE_GUEST_MEM_TYPE_UNKNOWN);
}

REGUESTMEMStatus re_guest_memory_unmap(REGUESTMEMORY *m,uint32_t base) {
 if(!m) return RE_GUEST_MEM_E_ARGUMENT;
 for(size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
  REGUESTMEMRegion *r=&m->regions[i];
  if(r->active&&r->base==base) {
   memset(r,0,sizeof(*r));
   if(m->count) m->count--;
   return RE_GUEST_MEM_OK;
  }
 }
 return RE_GUEST_MEM_E_NOT_FOUND;
}

REGUESTMEMStatus re_guest_memory_resolve(const REGUESTMEMORY *m,
 uint32_t address,size_t length,uint32_t required,void **out) {
 uint64_t available,end;
 if(out) *out=NULL;
 if(!m||!out||!length||(required&~RE_GUEST_MEM_VALID_FLAGS))
  return RE_GUEST_MEM_E_ARGUMENT;
 available=RE_GUEST_ADDRESS_SPACE_SIZE-(uint64_t)address;
 if((uint64_t)length>available) return RE_GUEST_MEM_E_RANGE;
 end=(uint64_t)address+(uint64_t)length;
 for(size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
  const REGUESTMEMRegion *r=&m->regions[i];
  uint64_t rend;
  if(!r->active) continue;
  rend=(uint64_t)r->base+r->size;
  if(address<r->base||end>rend) continue;
  if((r->permissions&required)!=required) return RE_GUEST_MEM_E_PERMISSION;
  *out=r->host+(size_t)(address-r->base);
  return RE_GUEST_MEM_OK;
 }
 return RE_GUEST_MEM_E_NOT_FOUND;
}

REGUESTMEMStatus re_guest_memory_query_type(const REGUESTMEMORY *m,
 uint32_t address,size_t length,REGUESTMEMType *type) {
 uint64_t available,end;
 if(type) *type=RE_GUEST_MEM_TYPE_UNKNOWN;
 if(!m||!type||!length) return RE_GUEST_MEM_E_ARGUMENT;
 available=RE_GUEST_ADDRESS_SPACE_SIZE-(uint64_t)address;
 if((uint64_t)length>available) return RE_GUEST_MEM_E_RANGE;
 end=(uint64_t)address+(uint64_t)length;
 for(size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
  const REGUESTMEMRegion *r=&m->regions[i];
  uint64_t rend;
  if(!r->active) continue;
  rend=(uint64_t)r->base+r->size;
  if(address<r->base||end>rend) continue;
  *type=r->memory_type;
  return RE_GUEST_MEM_OK;
 }
 return RE_GUEST_MEM_E_NOT_FOUND;
}

size_t re_guest_memory_region_count(const REGUESTMEMORY *m) {
 return m?m->count:0u;
}
