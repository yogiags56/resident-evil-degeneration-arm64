#include "re_guest_memory_bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
 REGUESTMEMORY m; REGUESTMEMBUS bus; uint8_t code[16]={0x12,0x34,0x56,0x78},ro[8]={0xaa},rw[8]={0};
 uint8_t b=0; uint16_t h=0; uint32_t w=0;
 re_guest_memory_init(&m);
 assert(re_guest_memory_map(&m,0x1000,sizeof(code),code,RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map(&m,0x2000,sizeof(ro),ro,RE_GUEST_MEM_READ)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map(&m,0x3000,sizeof(rw),rw,RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
 re_guest_memory_bus_init(&bus,&m);
 assert(re_guest_memory_fetch8(&bus,0x1000,&b)==RE_GUEST_MEM_OK&&b==0x12);
 assert(re_guest_memory_fetch16(&bus,0x1000,&h)==RE_GUEST_MEM_OK&&h==0x3412);
 assert(re_guest_memory_fetch32(&bus,0x1000,&w)==RE_GUEST_MEM_OK&&w==0x78563412u);
 assert(re_guest_memory_read8(&bus,0x2000,&b)==RE_GUEST_MEM_OK&&b==0xaa);
 assert(re_guest_memory_fetch8(&bus,0x2000,&b)==RE_GUEST_MEM_E_PERMISSION);
 assert(re_guest_memory_write8(&bus,0x2000,1)==RE_GUEST_MEM_E_PERMISSION);
 assert(re_guest_memory_write32(&bus,0x3000,0xdeadbeefu)==RE_GUEST_MEM_OK);
 assert(rw[0]==0xef&&rw[1]==0xbe&&rw[2]==0xad&&rw[3]==0xde);
 assert(re_guest_memory_read32(&bus,0x3000,&w)==RE_GUEST_MEM_OK&&w==0xdeadbeefu);
 assert(re_guest_memory_read8(&bus,0x1800,&b)==RE_GUEST_MEM_E_NOT_FOUND);
 assert(re_guest_memory_read32(&bus,0x3006,&w)==RE_GUEST_MEM_E_NOT_FOUND);
 assert(re_guest_memory_read16(&bus,0x3001,&h)==RE_GUEST_MEM_E_RANGE);
 assert(re_guest_memory_write32(&bus,0x3002,0)==RE_GUEST_MEM_E_RANGE);
 assert(re_guest_memory_fetch32(&bus,0x100e,&w)==RE_GUEST_MEM_E_NOT_FOUND);
 assert(re_guest_memory_fetch16(&bus,0x100f,&h)==RE_GUEST_MEM_E_RANGE);
 assert(re_guest_memory_read8(&bus,0xffffffffu,&b)==RE_GUEST_MEM_E_NOT_FOUND);
 assert(re_guest_memory_fetch8(&bus,0x1000,NULL)==RE_GUEST_MEM_E_ARGUMENT);
 puts("Guest memory bus tests: PASS (18 assertions)");
 return 0;
}
