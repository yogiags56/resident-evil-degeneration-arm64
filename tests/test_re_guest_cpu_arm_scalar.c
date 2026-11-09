#include "re_guest_cpu_arm_scalar.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void init(REGUESTMEMORY *map,REGUESTALIGNMENTPOLICY *policy,
 REGUESTCPUSCALAR *scalar,uint8_t *data,size_t size) {
 re_guest_memory_init(map);
 re_guest_alignment_policy_init(policy);
 assert(re_guest_memory_map_ex(map,0x8000u,size,data,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 re_guest_cpu_scalar_init(scalar,map,policy);
}

int main(void) {
 REGUESTMEMORY map;
 REGUESTALIGNMENTPOLICY policy;
 REGUESTCPUSCALAR scalar;
 REGUESTCPU cpu;
 uint8_t data[64];
 memset(data,0,sizeof(data));
 init(&map,&policy,&scalar,data,sizeof(data));
 re_guest_cpu_reset(&cpu,0x1000u,0);
 cpu.r[1]=0x8000u;

 /* Word load/store, AL, immediate, pre-index, no writeback. */
 data[0]=0x78;data[1]=0x56;data[2]=0x34;data[3]=0x12;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5910000u)==RE_CPU_OK);
 assert(cpu.r[0]==0x12345678u);
 cpu.r[0]=0xa1b2c3d4u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5810004u)==RE_CPU_OK);
 assert(data[4]==0xd4u&&data[5]==0xc3u&&data[6]==0xb2u&&data[7]==0xa1u);

 /* Byte load/store. */
 data[8]=0x80u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5d12008u)==RE_CPU_OK);
 assert(cpu.r[2]==0x80u);
 cpu.r[2]=0x5au;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5c12009u)==RE_CPU_OK);
 assert(data[9]==0x5au);

 /* Halfword unsigned load/store. */
 data[10]=0x34u;data[11]=0x12u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe1d130bau)==RE_CPU_OK);
 assert(cpu.r[3]==0x1234u);
 cpu.r[3]=0xabcdU;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe1c130bcu)==RE_CPU_OK);
 assert(data[12]==0xcdu&&data[13]==0xabu);

 /* Signed byte and signed halfword loads. */
 data[14]=0x80u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe1d140deu)==RE_CPU_OK);
 assert(cpu.r[4]==0xffffff80u);
 data[16]=0x00u;data[17]=0x80u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe1d151f0u)==RE_CPU_OK);
 assert(cpu.r[5]==0xffff8000u);

 /* SCTLR.A clear permits ordinary unaligned Normal-memory scalar transfers. */
 re_guest_alignment_policy_set_sctlr_a(&policy,0u);
 data[21]=0x11u;data[22]=0x22u;data[23]=0x33u;data[24]=0x44u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5916015u)==RE_CPU_OK);
 assert(cpu.r[6]==0x44332211u);
 cpu.r[6]=0x88776655u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5816016u)==RE_CPU_OK);
 assert(data[22]==0x55u&&data[23]==0x66u&&data[24]==0x77u&&data[25]==0x88u);

 /* Restore strict mode: unaligned word load faults, Rd is preserved. */
 re_guest_alignment_policy_set_sctlr_a(&policy,1u);
 cpu.r[7]=0xfeedfaceu;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5917015u)==RE_CPU_E_ALIGNMENT);
 assert(cpu.r[7]==0xfeedfaceu);

 /* Negative offset is supported and uses 32-bit guest arithmetic. */
 cpu.r[1]=0x8004u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5110004u)==RE_CPU_OK);
 assert(cpu.r[0]==0x12345678u);

 /* Failed store/load must not partially mutate guest state. */
 cpu.r[1]=0x9000u;cpu.r[0]=0x55667788u;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5810000u)==RE_CPU_E_MEMORY);
 cpu.r[2]=0xdecafbadU;
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5912000u)==RE_CPU_E_MEMORY);
 assert(cpu.r[2]==0xdecafbadU);

 /* Reject unsupported condition, post-index, writeback, PC and register offset. */
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0x15910000u)==RE_CPU_E_UNIMPLEMENTED);
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe4910000u)==RE_CPU_E_UNIMPLEMENTED);
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe5b10000u)==RE_CPU_E_UNIMPLEMENTED);
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe59f0000u)==RE_CPU_E_UNIMPLEMENTED);
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe7910002u)==RE_CPU_E_UNIMPLEMENTED);
 assert(re_guest_cpu_arm_scalar_transfer(&cpu,&scalar,0xe1d100d0u)==RE_CPU_E_UNIMPLEMENTED);
 assert(re_guest_cpu_arm_scalar_transfer(NULL,&scalar,0xe5910000u)==RE_CPU_E_ARGUMENT);

 puts("A32 scalar transfer integration tests: PASS");
 return 0;
}
