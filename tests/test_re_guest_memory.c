#include "re_guest_memory.h"
#include "re_guest_memory_view.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void) {
 REGUESTMEMORY m,full; uint8_t code[64]={0},data[128]={0},last=0,slots[RE_GUEST_MEMORY_MAX_REGIONS]={0};
 void *p=(void *)(uintptr_t)1; REGUESTMEM view;
 re_guest_memory_init(&m);
 assert(re_guest_memory_map(&m,0x1000u,sizeof(code),code,RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map(&m,0x2000u,sizeof(data),data,RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_region_count(&m)==2u);
 assert(re_guest_memory_resolve(&m,0x1004u,4,RE_GUEST_MEM_EXEC,&p)==RE_GUEST_MEM_OK&&p==code+4);
 assert(re_guest_memory_resolve(&m,0x2008u,8,RE_GUEST_MEM_WRITE,&p)==RE_GUEST_MEM_OK&&p==data+8);
 assert(re_guest_memory_resolve(&m,0x1000u,4,RE_GUEST_MEM_WRITE,&p)==RE_GUEST_MEM_E_PERMISSION&&p==NULL);
 assert(re_guest_memory_resolve(&m,0x103fu,2,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_NOT_FOUND);
 assert(re_guest_memory_resolve(&m,0xFFFFFFFEu,4,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_RANGE);
 assert(re_guest_memory_map(&m,0x1030u,16,data,RE_GUEST_MEM_READ)==RE_GUEST_MEM_E_OVERLAP);
 assert(re_guest_memory_map(&m,0xFFFFFFF0u,32,data,RE_GUEST_MEM_READ)==RE_GUEST_MEM_E_RANGE);
 assert(re_guest_memory_map(&m,0,1,data,0)==RE_GUEST_MEM_E_ARGUMENT);
 assert(re_guest_memory_map(&m,0,1,data,8)==RE_GUEST_MEM_E_ARGUMENT);
 assert(re_guest_memory_resolve(&m,0,0,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_ARGUMENT);
 assert(re_guest_memory_resolve(&m,0,(size_t)-1,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_RANGE);
 assert(re_guest_memory_region_view(&m,0x1000u,RE_GUEST_MEM_EXEC,&view)==RE_GUEST_MEM_OK);
 assert(view.bytes==code&&view.size==sizeof(code)&&view.guest_base==0x1000u);
 assert(re_guest_memory_region_view(&m,0x1000u,RE_GUEST_MEM_WRITE,&view)==RE_GUEST_MEM_E_PERMISSION&&view.bytes==NULL);
 assert(re_guest_memory_unmap(&m,0x1000u)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_resolve(&m,0x1000u,1,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_NOT_FOUND);
 assert(re_guest_memory_region_count(&m)==1u);
 assert(re_guest_memory_unmap(&m,0x1000u)==RE_GUEST_MEM_E_NOT_FOUND);
 assert(re_guest_memory_map(&m,0xFFFFFFFFu,1,&last,RE_GUEST_MEM_READ)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_resolve(&m,0xFFFFFFFFu,1,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_OK&&p==&last);
 assert(re_guest_memory_resolve(&m,0xFFFFFFFFu,2,RE_GUEST_MEM_READ,&p)==RE_GUEST_MEM_E_RANGE);
 re_guest_memory_init(&full);
 for(size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++)
  assert(re_guest_memory_map(&full,(uint32_t)(i*2u),1,&slots[i],RE_GUEST_MEM_READ)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_region_count(&full)==RE_GUEST_MEMORY_MAX_REGIONS);
 assert(re_guest_memory_map(&full,0x1000u,1,&last,RE_GUEST_MEM_READ)==RE_GUEST_MEM_E_CAPACITY);
 puts("Guest memory manager tests: PASS (24 assertions)");
 return 0;
}
