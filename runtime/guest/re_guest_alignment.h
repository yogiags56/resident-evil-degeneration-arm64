#ifndef RE_GUEST_ALIGNMENT_H
#define RE_GUEST_ALIGNMENT_H
#include "re_guest_memory.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Mirrors the guest SCTLR.A control bit. Until system-register emulation
 * supplies the value, callers must initialize this policy as strict. */
typedef struct {
 uint8_t alignment_check_enabled;
} REGUESTALIGNMENTPOLICY;

typedef enum {
 RE_GUEST_ACCESS_BYTE=0,
 RE_GUEST_ACCESS_HALFWORD,
 RE_GUEST_ACCESS_WORD,
 RE_GUEST_ACCESS_EXCLUSIVE,
 RE_GUEST_ACCESS_DOUBLEWORD,
 RE_GUEST_ACCESS_MULTIPLE,
 RE_GUEST_ACCESS_SWP,
 RE_GUEST_ACCESS_COPROCESSOR
} REGUESTACCESSCLASS;

typedef enum {
 RE_GUEST_ALIGN_OK=0,
 RE_GUEST_ALIGN_E_ARGUMENT=-1,
 RE_GUEST_ALIGN_E_FAULT=-2,
 RE_GUEST_ALIGN_E_MEMORY=-3,
 RE_GUEST_ALIGN_E_UNSUPPORTED=-4
} REGUESTALIGNMENTSTATUS;

void re_guest_alignment_policy_init(REGUESTALIGNMENTPOLICY *policy);
void re_guest_alignment_policy_set_sctlr_a(REGUESTALIGNMENTPOLICY *policy,
 uint8_t enabled);
REGUESTALIGNMENTSTATUS re_guest_alignment_check(
 const REGUESTMEMORY *memory,
 const REGUESTALIGNMENTPOLICY *policy,
 uint32_t address,
 size_t width,
 REGUESTACCESSCLASS access_class);
#ifdef __cplusplus
}
#endif
#endif
