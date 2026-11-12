#ifndef RE_GUEST_ELF32_RELOC_PLAN_H
#define RE_GUEST_ELF32_RELOC_PLAN_H

#include "re_guest_elf32_link.h"
#include "re_guest_relocation_apply.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RE_GUEST_ELF32_RELOC_PLAN_OK = 0,
    RE_GUEST_ELF32_RELOC_PLAN_E_ARGUMENT = -660,
    RE_GUEST_ELF32_RELOC_PLAN_E_CAPACITY = -661,
    RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL = -662,
    RE_GUEST_ELF32_RELOC_PLAN_E_UNRESOLVED = -663,
    RE_GUEST_ELF32_RELOC_PLAN_E_UNSUPPORTED = -664,
    RE_GUEST_ELF32_RELOC_PLAN_E_RANGE = -665,
    RE_GUEST_ELF32_RELOC_PLAN_E_ALIGNMENT = -666
} REGUESTELF32RELOCPLANStatus;

/*
 * Convert decoded ELF32 ARM relocation records into the existing relocation
 * engine's caller-prepared form. Defined symbols are interpreted within this
 * image; undefined symbols resolve only against the caller's already-loaded,
 * already-addressed provider list. Unresolved weak imports become zero.
 *
 * This is deliberately not a dynamic linker: no DT_NEEDED loading, symbol
 * interposition, GNU versioning, TLS, COPY, IFUNC, PLT lazy binding, or load
 * allocation. Versioned dynamic symbol metadata is rejected until decoded.
 * Output is transactional and remains unchanged on error.
 */
REGUESTELF32RELOCPLANStatus re_guest_elf32_prepare_relocations(
    const REGUESTELF32DYNAMIC *dynamic,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    const REGUESTELF32LINKRELOCATION *input, size_t input_count,
    const REGUESTSYMBOL *loaded_providers, size_t provider_count,
    uint32_t load_bias,
    REGUESTRELOCATION *output, size_t output_capacity,
    size_t *failed_index);

#ifdef __cplusplus
}
#endif
#endif
