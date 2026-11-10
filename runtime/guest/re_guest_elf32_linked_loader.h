#ifndef RE_GUEST_ELF32_LINKED_LOADER_H
#define RE_GUEST_ELF32_LINKED_LOADER_H

#include "re_guest_elf32_loader.h"
#include "re_guest_elf32_reloc_plan.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    REGUESTELF32LINKSYMBOL *symbols;
    size_t symbol_capacity;
    REGUESTELF32LINKRELOCATION *decoded_relocations;
    size_t decoded_relocation_capacity;
    REGUESTRELOCATION *prepared_relocations;
    size_t prepared_relocation_capacity;
} REGUESTELF32LINKWORKSPACE;

typedef struct {
    REGUESTELF32LOADRESULT load;
    REGUESTELF32LINKINFO link;
    uint32_t dynamic_dependency_count;
} REGUESTELF32LINKEDLOADRESULT;

typedef enum {
    RE_GUEST_ELF32_LINKED_LOAD_OK = 0,
    RE_GUEST_ELF32_LINKED_LOAD_E_ARGUMENT = -680,
    RE_GUEST_ELF32_LINKED_LOAD_E_IMAGE = -681,
    RE_GUEST_ELF32_LINKED_LOAD_E_DYNAMIC = -682,
    RE_GUEST_ELF32_LINKED_LOAD_E_METADATA = -683,
    RE_GUEST_ELF32_LINKED_LOAD_E_RELOCATION_PLAN = -684,
    RE_GUEST_ELF32_LINKED_LOAD_E_LOAD = -685
} REGUESTELF32LINKEDLOADStatus;

/*
 * Caller-driven single-image linked load coordinator:
 * parse ELF/PT_DYNAMIC, decode symbols and relocations, resolve undefined
 * symbols against caller-supplied loaded providers, prepare relocation
 * records, then invoke the transactional ELF32 image loader.
 *
 * All scratch arrays and segment buffers are caller-owned. No filesystem
 * access, DT_NEEDED provider loading, host allocation, symbol interposition,
 * GNU versioning, constructors, or guest execution occurs.
 */
REGUESTELF32LINKEDLOADStatus re_guest_elf32_load_linked_image(
    REGUESTMEMORY *memory,
    const uint8_t *elf_bytes, size_t elf_size, uint32_t load_bias,
    const REGUESTELF32LOADBUFFER *buffers, size_t buffer_count,
    const REGUESTSYMBOL *loaded_providers, size_t provider_count,
    const REGUESTELF32LINKWORKSPACE *workspace,
    REGUESTELF32LINKEDLOADRESULT *result,
    size_t *failed_index, int *inner_status);

#ifdef __cplusplus
}
#endif
#endif
