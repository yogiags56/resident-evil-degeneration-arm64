#ifndef RE_GUEST_ELF_MATERIALIZE_H
#define RE_GUEST_ELF_MATERIALIZE_H

#include "re_guest_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t guest_address;
    uint64_t file_offset;
    uint64_t file_size;
    uint64_t memory_size;
} REGUESTELFSEGMENT;

typedef enum {
    RE_GUEST_ELF_MATERIALIZE_OK = 0,
    RE_GUEST_ELF_MATERIALIZE_E_ARGUMENT = -200,
    RE_GUEST_ELF_MATERIALIZE_E_SOURCE_RANGE = -201,
    RE_GUEST_ELF_MATERIALIZE_E_SIZE = -202,
    RE_GUEST_ELF_MATERIALIZE_E_GUEST_RANGE = -203,
    RE_GUEST_ELF_MATERIALIZE_E_OVERLAP = -204,
    RE_GUEST_ELF_MATERIALIZE_E_PERMISSION = -205
} REGUESTELFMaterializeStatus;

/*
 * Preflight every segment before modifying guest memory, then copy file bytes
 * and zero the BSS tail. Destination mappings must already exist and be
 * writable. This function does not allocate, map, relocate, or execute.
 */
REGUESTELFMaterializeStatus re_guest_elf_materialize(
    REGUESTMEMORY *memory,
    const uint8_t *elf_bytes,
    size_t elf_size,
    const REGUESTELFSEGMENT *segments,
    size_t segment_count,
    size_t *failed_index);

#ifdef __cplusplus
}
#endif
#endif
