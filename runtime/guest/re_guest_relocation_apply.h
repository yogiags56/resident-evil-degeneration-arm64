#ifndef RE_GUEST_RELOCATION_APPLY_H
#define RE_GUEST_RELOCATION_APPLY_H

#include "re_guest_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ELF32 ARM relocation type numbers from the ARM ELF ABI. */
enum {
    RE_ARM_R_ABS32 = 2,
    RE_ARM_R_GLOB_DAT = 21,
    RE_ARM_R_JUMP_SLOT = 22,
    RE_ARM_R_RELATIVE = 23
};

typedef struct {
    uint32_t target;
    uint32_t type;
    uint32_t symbol_index;
    uint32_t symbol_value;
    uint32_t load_bias;
    uint32_t addend;
    uint8_t has_explicit_addend; /* RELA=1; REL=0 and reads target word. */
    uint8_t symbol_resolved;     /* Required for ABS32/GLOB_DAT/JUMP_SLOT. */
} REGUESTRELOCATION;

typedef enum {
    RE_GUEST_RELOC_APPLY_OK = 0,
    RE_GUEST_RELOC_APPLY_E_ARGUMENT = -400,
    RE_GUEST_RELOC_APPLY_E_UNSUPPORTED = -401,
    RE_GUEST_RELOC_APPLY_E_SYMBOL = -402,
    RE_GUEST_RELOC_APPLY_E_TARGET = -403,
    RE_GUEST_RELOC_APPLY_E_PERMISSION = -404,
    RE_GUEST_RELOC_APPLY_E_ALIGNMENT = -405,
    RE_GUEST_RELOC_APPLY_E_DUPLICATE = -406,
    RE_GUEST_RELOC_APPLY_E_RELATIVE_SYMBOL = -407
} REGUESTRELOCApplyStatus;

/*
 * Apply a bounded set of already-audited ELF32 ARM relocations atomically.
 * The caller supplies resolved symbol values; this routine never searches
 * symbol tables or loads libraries. REL implicit addends are read from the
 * original target word. All targets must be aligned, writable 32-bit words.
 *
 * The caller must provide exclusive access to guest memory for the duration
 * of this call. Preflight completes before any target word is changed.
 * Arithmetic follows 32-bit ELF relocation word semantics.
 */
REGUESTRELOCApplyStatus re_guest_relocations_apply(
    REGUESTMEMORY *memory,
    const REGUESTRELOCATION *relocations,
    size_t count,
    size_t *failed_index);

#ifdef __cplusplus
}
#endif
#endif
