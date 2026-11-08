#include "re_guest_memory_bus.h"
static REGUESTMEMStatus resolve(const REGUESTMEMBUS *bus,uint32_t address,size_t width,uint32_t permission,void **out) {
    if(out)*out=0;
    if(!bus||!bus->memory||!out)return RE_GUEST_MEM_E_ARGUMENT;
    return re_guest_memory_resolve(bus->memory,address,width,permission,out);
}
static REGUESTMEMStatus get(const REGUESTMEMBUS *bus,uint32_t a,size_t n,uint32_t permission,uint32_t *v) {
    void *p=0; REGUESTMEMStatus s;
    if(!v)return RE_GUEST_MEM_E_ARGUMENT;
    *v=0; s=resolve(bus,a,n,permission,&p); if(s!=RE_GUEST_MEM_OK)return s;
    const uint8_t *b=(const uint8_t *)p;
    if(n==1)*v=b[0];
    else if(n==2)*v=(uint32_t)b[0]|((uint32_t)b[1]<<8);
    else *v=(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
    return RE_GUEST_MEM_OK;
}
static REGUESTMEMStatus put(REGUESTMEMBUS *bus,uint32_t a,size_t n,uint32_t value) {
    void *p=0; REGUESTMEMStatus s=resolve(bus,a,n,RE_GUEST_MEM_WRITE,&p);
    if(s!=RE_GUEST_MEM_OK)return s;
    uint8_t *b=(uint8_t *)p;
    for(size_t i=0;i<n;i++)b[i]=(uint8_t)(value>>(i*8u));
    return RE_GUEST_MEM_OK;
}
void re_guest_memory_bus_init(REGUESTMEMBUS *bus,const REGUESTMEMORY *memory) {
    if(bus)bus->memory=memory;
}
REGUESTMEMStatus re_guest_memory_fetch8(const REGUESTMEMBUS*b,uint32_t a,uint8_t*v){uint32_t x;REGUESTMEMStatus s;if(!v)return RE_GUEST_MEM_E_ARGUMENT;*v=0;s=get(b,a,1,RE_GUEST_MEM_EXEC,&x);if(s==RE_GUEST_MEM_OK)*v=(uint8_t)x;return s;}
REGUESTMEMStatus re_guest_memory_fetch16(const REGUESTMEMBUS*b,uint32_t a,uint16_t*v){uint32_t x;REGUESTMEMStatus s;if(!v)return RE_GUEST_MEM_E_ARGUMENT;if(a&1u)return RE_GUEST_MEM_E_RANGE;*v=0;s=get(b,a,2,RE_GUEST_MEM_EXEC,&x);if(s==RE_GUEST_MEM_OK)*v=(uint16_t)x;return s;}
REGUESTMEMStatus re_guest_memory_fetch32(const REGUESTMEMBUS*b,uint32_t a,uint32_t*v){if(a&3u){if(v)*v=0;return RE_GUEST_MEM_E_RANGE;}return get(b,a,4,RE_GUEST_MEM_EXEC,v);}
REGUESTMEMStatus re_guest_memory_read8(const REGUESTMEMBUS*b,uint32_t a,uint8_t*v){uint32_t x;REGUESTMEMStatus s;if(!v)return RE_GUEST_MEM_E_ARGUMENT;*v=0;s=get(b,a,1,RE_GUEST_MEM_READ,&x);if(s==RE_GUEST_MEM_OK)*v=(uint8_t)x;return s;}
REGUESTMEMStatus re_guest_memory_read16(const REGUESTMEMBUS*b,uint32_t a,uint16_t*v){uint32_t x;REGUESTMEMStatus s;if(!v)return RE_GUEST_MEM_E_ARGUMENT;if(a&1u)return RE_GUEST_MEM_E_RANGE;*v=0;s=get(b,a,2,RE_GUEST_MEM_READ,&x);if(s==RE_GUEST_MEM_OK)*v=(uint16_t)x;return s;}
REGUESTMEMStatus re_guest_memory_read32(const REGUESTMEMBUS*b,uint32_t a,uint32_t*v){if(a&3u){if(v)*v=0;return RE_GUEST_MEM_E_RANGE;}return get(b,a,4,RE_GUEST_MEM_READ,v);}
REGUESTMEMStatus re_guest_memory_write8(REGUESTMEMBUS*b,uint32_t a,uint8_t v){return put(b,a,1,v);}
REGUESTMEMStatus re_guest_memory_write16(REGUESTMEMBUS*b,uint32_t a,uint16_t v){if(a&1u)return RE_GUEST_MEM_E_RANGE;return put(b,a,2,v);}
REGUESTMEMStatus re_guest_memory_write32(REGUESTMEMBUS*b,uint32_t a,uint32_t v){if(a&3u)return RE_GUEST_MEM_E_RANGE;return put(b,a,4,v);}
