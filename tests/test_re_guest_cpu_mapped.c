#include "re_guest_cpu.h"
#include "re_guest_memory_bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void put32(uint8_t *p,uint32_t v) {
 p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}

static void test_thumb_mapped(void) {
 REGUESTMEMORY memory;REGUESTMEMBUS bus;REGUESTCPU cpu;
 uint8_t code[4]={0x09,0x20,0x03,0x30};
 re_guest_memory_init(&memory);
 assert(re_guest_memory_map(&memory,0x4000,sizeof(code),code,
   RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
 re_guest_memory_bus_init(&bus,&memory);
 re_guest_cpu_reset(&cpu,0x4000,1);
 assert(re_guest_cpu_step_mapped(&cpu,&bus)==RE_CPU_OK);
 assert(cpu.r[0]==9&&cpu.r[15]==0x4002);
 assert(re_guest_cpu_step_mapped(&cpu,&bus)==RE_CPU_OK);
 assert(cpu.r[0]==12&&cpu.r[15]==0x4004);
}
static void test_thumb2_fetch_permission(void) {
 REGUESTMEMORY memory;REGUESTMEMBUS bus;REGUESTCPU cpu;
 uint8_t first[2]={0x40,0xf2},second[2]={0x2a,0x00};
 re_guest_memory_init(&memory);
 assert(re_guest_memory_map(&memory,0x5000,2,first,
   RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map(&memory,0x5002,2,second,
   RE_GUEST_MEM_READ)==RE_GUEST_MEM_OK);
 re_guest_memory_bus_init(&bus,&memory);
 re_guest_cpu_reset(&cpu,0x5000,1);
 assert(re_guest_cpu_step_mapped(&cpu,&bus)==RE_CPU_E_PERMISSION);
 assert(cpu.r[0]==0&&cpu.r[15]==0x5000&&cpu.steps==0);
}

int main(void) {
 REGUESTMEMORY memory;REGUESTMEMBUS bus;REGUESTCPU cpu;
 uint8_t code[12]={0},data[16]={0};
 /* MOV r0,#42 ; LDR r1,[r2] ; STR r0,[r2,#4] */
 put32(code,0xe3a0002au);put32(code+4,0xe5921000u);put32(code+8,0xe5820004u);
 put32(data,0x12345678u);
 re_guest_memory_init(&memory);
 assert(re_guest_memory_map(&memory,0x1000,sizeof(code),code,
   RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map(&memory,0x2000,sizeof(data),data,
   RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
 re_guest_memory_bus_init(&bus,&memory);
 re_guest_cpu_reset(&cpu,0x1000,0);cpu.r[2]=0x2000;
 assert(re_guest_cpu_step_mapped(&cpu,&bus)==RE_CPU_OK);
 assert(cpu.r[0]==42&&cpu.r[15]==0x1004);
 assert(re_guest_cpu_step_mapped(&cpu,&bus)==RE_CPU_OK);
 assert(cpu.r[1]==0x12345678u&&cpu.r[15]==0x1008);
 assert(re_guest_cpu_step_mapped(&cpu,&bus)==RE_CPU_OK);
 assert(data[4]==42&&data[5]==0&&data[6]==0&&data[7]==0);
 assert(cpu.r[15]==0x100c&&cpu.steps==3);

 /* Data memory cannot be executed; code memory cannot be written. */
 re_guest_cpu_reset(&cpu,0x2000,0);
 assert(re_guest_cpu_step_mapped(&cpu,&bus)==RE_CPU_E_PERMISSION);
 assert(re_guest_memory_write32(&bus,0x1000,0)==RE_GUEST_MEM_E_PERMISSION);
 /* A missing region is a guest memory fault, not host pointer access. */
 re_guest_cpu_reset(&cpu,0x3000,0);
 assert(re_guest_cpu_step_mapped(&cpu,&bus)==RE_CPU_E_MEMORY);
 test_thumb_mapped();test_thumb2_fetch_permission();
 puts("Guest CPU mapped multi-region execution tests: PASS");
 return 0;
}
