#include "re_guest_cpu_arm_scalar.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void init_scalar(REGUESTMEMORY *map,
 REGUESTALIGNMENTPOLICY *policy,REGUESTCPUSCALAR *scalar) {
 re_guest_memory_init(map);
 re_guest_alignment_policy_init(policy);
 re_guest_cpu_scalar_init(scalar,map,policy);
}

int main(void) {
 REGUESTMEMORY map;
 REGUESTALIGNMENTPOLICY policy;
 REGUESTCPUSCALAR scalar;
 REGUESTCPU cpu;
 uint8_t bytes[8]={0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
 uint8_t before[8];

 memset(&cpu,0,sizeof(cpu));
 init_scalar(&map,&policy,&scalar);
 cpu.r[1]=0x8000u;
 cpu.r[0]=0xaabbccddu;

 /* Read-only Normal memory: LDR works, STR is rejected without mutation. */
 assert(re_guest_memory_map_ex(&map,0x8000u,sizeof(bytes),bytes,
  RE_GUEST_MEM_READ,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 memcpy(before,bytes,sizeof(bytes));
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5912000u)==RE_CPU_OK);
 assert(cpu.r[2]==0x44332211u);
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5810000u)==RE_CPU_E_PERMISSION);
 assert(memcmp(bytes,before,sizeof(bytes))==0);

 /* A word access spanning two adjacent regions must not be partially applied. */
 re_guest_memory_init(&map);
 re_guest_cpu_scalar_init(&scalar,&map,&policy);
 memset(bytes,0x5a,sizeof(bytes));
 memcpy(before,bytes,sizeof(bytes));
 assert(re_guest_memory_map_ex(&map,0x8000u,2u,bytes,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&map,0x8002u,2u,bytes+2,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 cpu.r[1]=0x8000u;
 cpu.r[0]=0xdeadbeefu;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5810000u)==RE_CPU_E_MEMORY);
 assert(memcmp(bytes,before,sizeof(bytes))==0);
 cpu.r[3]=0x12345678u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5913000u)==RE_CPU_E_MEMORY);
 assert(cpu.r[3]==0x12345678u);

 /* 32-bit guest address wrap and subtraction underflow fail closed. */
 re_guest_memory_init(&map);
 re_guest_cpu_scalar_init(&scalar,&map,&policy);
 assert(re_guest_memory_map_ex(&map,0x8000u,sizeof(bytes),bytes,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 cpu.r[1]=0xfffffff0u;
 cpu.r[4]=0xfacecafeu;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5914020u)==RE_CPU_E_MEMORY);
 assert(cpu.r[4]==0xfacecafeu);
 cpu.r[1]=0x00000010u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5114020u)==RE_CPU_E_MEMORY);
 assert(cpu.r[4]==0xfacecafeu);

 /* Reject unsupported forms without changing CPU state or memory. */
 cpu.r[1]=0x8000u;
 memcpy(before,bytes,sizeof(bytes));
 {
  static const uint32_t invalid[]={
   0x15910000u, /* conditional */
   0xe4910000u, /* post-index */
   0xe5b10000u, /* writeback */
   0xe59f0000u, /* PC base */
   0xe591f000u, /* PC destination */
   0xe7910002u, /* register offset */
   0xe1d10090u, /* LDRD encoding */
   0xe1c100d0u, /* unsupported signed-byte store */
   0xe1c100f0u  /* unsupported signed-halfword store */
  };
  for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++) {
   uint32_t saved[16];
   memcpy(saved,cpu.r,sizeof(saved));
   assert(re_guest_cpu_arm_scalar_transfer(
    &cpu,&scalar,invalid[i])==RE_CPU_E_UNIMPLEMENTED);
   assert(memcmp(saved,cpu.r,sizeof(saved))==0);
   assert(memcmp(before,bytes,sizeof(bytes))==0);
  }
 }

 puts("A32 directed fault and decoder tests: PASS");
 return 0;
}
