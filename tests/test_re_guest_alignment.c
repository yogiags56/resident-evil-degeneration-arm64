#include "re_guest_alignment.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
 REGUESTMEMORY memory;
 REGUESTALIGNMENTPOLICY policy;
 uint8_t normal[16]={0},device[16]={0},unknown[16]={0};
 re_guest_memory_init(&memory);
 re_guest_alignment_policy_init(&policy);
 assert(policy.alignment_check_enabled==1u);
 assert(re_guest_memory_map_ex(&memory,0x1000u,sizeof(normal),normal,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_NORMAL)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map_ex(&memory,0x2000u,sizeof(device),device,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,RE_GUEST_MEM_TYPE_DEVICE)==RE_GUEST_MEM_OK);
 assert(re_guest_memory_map(&memory,0x3000u,sizeof(unknown),unknown,
  RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,2u,
  RE_GUEST_ACCESS_HALFWORD)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,4u,
  RE_GUEST_ACCESS_WORD)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x1000u,4u,
  RE_GUEST_ACCESS_WORD)==RE_GUEST_ALIGN_OK);
 re_guest_alignment_policy_set_sctlr_a(&policy,0u);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,2u,
  RE_GUEST_ACCESS_HALFWORD)==RE_GUEST_ALIGN_OK);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,4u,
  RE_GUEST_ACCESS_WORD)==RE_GUEST_ALIGN_OK);
 assert(re_guest_alignment_check(&memory,&policy,0x2001u,2u,
  RE_GUEST_ACCESS_HALFWORD)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x3001u,4u,
  RE_GUEST_ACCESS_WORD)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,4u,
  RE_GUEST_ACCESS_EXCLUSIVE)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,8u,
  RE_GUEST_ACCESS_DOUBLEWORD)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,8u,
  RE_GUEST_ACCESS_MULTIPLE)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,4u,
  RE_GUEST_ACCESS_SWP)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,4u,
  RE_GUEST_ACCESS_COPROCESSOR)==RE_GUEST_ALIGN_E_FAULT);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,1u,
  RE_GUEST_ACCESS_BYTE)==RE_GUEST_ALIGN_OK);
 assert(re_guest_alignment_check(&memory,&policy,0x1001u,4u,
  RE_GUEST_ACCESS_HALFWORD)==RE_GUEST_ALIGN_E_ARGUMENT);
 assert(re_guest_alignment_check(&memory,&policy,0x4001u,4u,
  RE_GUEST_ACCESS_WORD)==RE_GUEST_ALIGN_E_MEMORY);
 assert(re_guest_alignment_check(NULL,&policy,0x1000u,4u,
  RE_GUEST_ACCESS_WORD)==RE_GUEST_ALIGN_E_ARGUMENT);
 puts("Guest alignment policy tests: PASS (20 assertions)");
 return 0;
}
