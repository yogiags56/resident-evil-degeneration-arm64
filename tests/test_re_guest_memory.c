#include "re_guest_memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    REGUESTMEMORY m; uint8_t code[64]={0}, data[128]={0}; void *p=NULL;
    re_guest_memory_init(&m);
    assert(re_guest_memory_map(&m,0x1000u,sizeof(code),code,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&m,0x2000u,sizeof(data),data,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_region_count(&m)==2u);
    assert(re_guest_memory_resolve(&m,0x1004u,4,RE_GUEST_MEM_EXEC,&p)==RE_GUEST_MEM_OK);
    assert(p==code+4);
    assert(re_guest_memory_resolve(&m,0x2008u,8,RE_GUEST_MEM_WRITE,&p)==RE_GUEST_MEM_OK);
    assert(p==data+8);
    assert(re_guest_memory_resolve(&m,0x1000u,4,RE_GUEST_MEM_WRITE,&p)==RE_GUEST_MEM_E_PERMISSION);
    assert(re_guest_memory_resolve(&m,0x103fu,2,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_NOT_FOUND);
    assert(re_guest_memory_resolve(&m,0xFFFFFFFEu,4,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_RANGE);
    assert(re_guest_memory_map(&m,0x1030u,16,data,RE_GUEST_MEM_READ)==RE_GUEST_MEM_E_OVERLAP);
    assert(re_guest_memory_map(&m,0xFFFFFFF0u,32,data,RE_GUEST_MEM_READ)==RE_GUEST_MEM_E_RANGE);
    assert(re_guest_memory_unmap(&m,0x1000u)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_resolve(&m,0x1000u,1,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_NOT_FOUND);
    assert(re_guest_memory_region_count(&m)==1u);
    assert(re_guest_memory_unmap(&m,0x1000u)==RE_GUEST_MEM_E_NOT_FOUND);
    puts("Guest memory manager tests: PASS (12 assertions)");
    return 0;
}
