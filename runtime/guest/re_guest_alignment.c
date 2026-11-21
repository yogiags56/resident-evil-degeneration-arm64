#include "re_guest_alignment.h"

void re_guest_alignment_policy_init(REGUESTALIGNMENTPOLICY *p) {
 if(p) p->alignment_check_enabled=1u;
}

void re_guest_alignment_policy_set_sctlr_a(REGUESTALIGNMENTPOLICY *p,
 uint8_t enabled) {
 if(p) p->alignment_check_enabled=(uint8_t)(enabled?1u:0u);
}

static int is_scalar(REGUESTACCESSCLASS c) {
 return c==RE_GUEST_ACCESS_BYTE||c==RE_GUEST_ACCESS_HALFWORD||
        c==RE_GUEST_ACCESS_WORD;
}

REGUESTALIGNMENTSTATUS re_guest_alignment_check(
 const REGUESTMEMORY *memory,const REGUESTALIGNMENTPOLICY *policy,
 uint32_t address,size_t width,REGUESTACCESSCLASS access_class) {
 REGUESTMEMType type=RE_GUEST_MEM_TYPE_UNKNOWN;
 uint32_t alignment;
 REGUESTMEMStatus ms;

 if(!memory||!policy||!width||access_class<RE_GUEST_ACCESS_BYTE||
    access_class>RE_GUEST_ACCESS_COPROCESSOR)
  return RE_GUEST_ALIGN_E_ARGUMENT;
 if((access_class==RE_GUEST_ACCESS_BYTE&&width!=1u)||
    (access_class==RE_GUEST_ACCESS_HALFWORD&&width!=2u)||
    (access_class==RE_GUEST_ACCESS_WORD&&width!=4u))
  return RE_GUEST_ALIGN_E_ARGUMENT;
 if(width>8u||(width&(width-1u))!=0u)
  return RE_GUEST_ALIGN_E_ARGUMENT;

 alignment=(uint32_t)width;
 if((address&(alignment-1u))==0u) return RE_GUEST_ALIGN_OK;
 if(width==1u) return RE_GUEST_ALIGN_OK;

 /* Exclusive, doubleword, multiple, SWP, and coprocessor transfers
  * remain alignment-constrained even when SCTLR.A is clear. */
 if(!is_scalar(access_class)) return RE_GUEST_ALIGN_E_FAULT;
 if(policy->alignment_check_enabled) return RE_GUEST_ALIGN_E_FAULT;

 ms=re_guest_memory_query_type(memory,address,width,&type);
 if(ms!=RE_GUEST_MEM_OK) return RE_GUEST_ALIGN_E_MEMORY;
 if(type!=RE_GUEST_MEM_TYPE_NORMAL) return RE_GUEST_ALIGN_E_FAULT;
 return RE_GUEST_ALIGN_OK;
}
