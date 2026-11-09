#include "re_guest_cpu_scalar.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
 REGUESTMEMORY map;
 REGUESTALIGNMENTPOLICY policy;
 REGUESTCPUSCALAR scalar;
 uint8_t normal[16],device[8],unknown[8],readonly[8];
 uint8_t b=0; uint16_t h=0; uint32_t w=0;
 memset(normal,0,sizeof(normal)); memset(device,0,sizeof(device));
 memset(unknown,0,sizeof(unknown)); memset(readonly,0,sizeof(readonly));
 re_guest_memory_init(&map);
 re_guest_alignment_policy_init(&policy);
 assert(re_guest_memory_map_ex(&map,0x1000u,sizeof(normal),normal,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&map,0x2000u,sizeof(device),device,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_DEVICE)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map(&map,0x3000u,sizeof(unknown),unknown,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&map,0x4000u,sizeof(readonly),readonly,
  RE_GUEST_MEM_READ,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 re_guest_cpu_scalar_init(&scalar,&map,&policy);

 /* Naturally aligned access works under strict default policy. */
 assert(re_guest_cpu_scalar_write32(&scalar,0x1000u,0x12345678u)==RE_CPU_OK);
 assert(re_guest_cpu_scalar_read32(&scalar,0x1000u,&w)==RE_CPU_OK && w==0x12345678u);
 assert(re_guest_cpu_scalar_write16(&scalar,0x1004u,0xabcdU)==RE_CPU_OK);
 assert(re_guest_cpu_scalar_read16(&scalar,0x1004u,&h)==RE_CPU_OK && h==0xabcdU);

 /* Unaligned accesses fault while SCTLR.A is enabled. */
 assert(re_guest_cpu_scalar_read32(&scalar,0x1001u,&w)==RE_CPU_E_ALIGNMENT);
 assert(re_guest_cpu_scalar_write16(&scalar,0x1001u,0x7788u)==RE_CPU_E_ALIGNMENT);

 /* SCTLR.A clear permits byte-assembled access only to NORMAL memory. */
 re_guest_alignment_policy_set_sctlr_a(&policy,0u);
 assert(re_guest_cpu_scalar_write32(&scalar,0x1001u,0xa1b2c3d4u)==RE_CPU_OK);
 assert(normal[1]==0xd4u && normal[2]==0xc3u && normal[3]==0xb2u && normal[4]==0xa1u);
 assert(re_guest_cpu_scalar_read32(&scalar,0x1001u,&w)==RE_CPU_OK && w==0xa1b2c3d4u);
 assert(re_guest_cpu_scalar_write16(&scalar,0x1003u,0x5a6bu)==RE_CPU_OK);
 assert(normal[3]==0x6bu && normal[4]==0x5au);
 assert(re_guest_cpu_scalar_read16(&scalar,0x1003u,&h)==RE_CPU_OK && h==0x5a6bu);

 /* Byte access has no alignment restriction, even for DEVICE/UNKNOWN. */
 assert(re_guest_cpu_scalar_write8(&scalar,0x2001u,0xacu)==RE_CPU_OK);
 assert(re_guest_cpu_scalar_read8(&scalar,0x2001u,&b)==RE_CPU_OK && b==0xacu);

 /* Unaligned multi-byte accesses stay strict for DEVICE and UNKNOWN. */
 assert(re_guest_cpu_scalar_read16(&scalar,0x2001u,&h)==RE_CPU_E_ALIGNMENT);
 assert(re_guest_cpu_scalar_write32(&scalar,0x3001u,0xdeadbeefu)==RE_CPU_E_ALIGNMENT);

 /* Aligned writes to read-only memory fail without mutation. */
 assert(re_guest_cpu_scalar_write32(&scalar,0x4000u,0x11223344u)==RE_CPU_E_PERMISSION);
 assert(readonly[0]==0u && readonly[1]==0u && readonly[2]==0u && readonly[3]==0u);

 /* Unaligned access cannot cross a region boundary. */
 assert(re_guest_cpu_scalar_read32(&scalar,0x100du,&w)==RE_CPU_E_MEMORY);
 assert(re_guest_cpu_scalar_write32(&scalar,0x100du,0xffffffffu)==RE_CPU_E_MEMORY);

 /* Failed loads preserve caller destination. */
 w=0xfeedfaceu;
 assert(re_guest_cpu_scalar_read32(&scalar,0x5000u,&w)==RE_CPU_E_MEMORY);
 assert(w==0xfeedfaceu);

 /* Invalid arguments and address-space overflow fail closed. */
 assert(re_guest_cpu_scalar_read16(&scalar,0x1000u,NULL)==RE_CPU_E_ARGUMENT);
 assert(re_guest_cpu_scalar_read32(NULL,0x1000u,&w)==RE_CPU_E_ARGUMENT);
 assert(re_guest_cpu_scalar_read32(&scalar,0xffffffffu,&w)==RE_CPU_E_ALIGNMENT);

 /* Strict policy can be restored. */
 re_guest_alignment_policy_set_sctlr_a(&policy,1u);
 assert(re_guest_cpu_scalar_read16(&scalar,0x1001u,&h)==RE_CPU_E_ALIGNMENT);

 puts("Guest CPU scalar transfer tests: PASS");
 return 0;
}
