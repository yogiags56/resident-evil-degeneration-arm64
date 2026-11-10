#ifndef RE_GUEST_ELF32_LOADER_H
#define RE_GUEST_ELF32_LOADER_H

#include "re_guest_elf32_dynamic.h"
#include "re_guest_relocation_apply.h"
#include "re_guest_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_ELF32_MAX_LOAD_SEGMENTS RE_GUEST_MEMORY_MAX_REGIONS
#define RE_GUEST_ELF32_RELOCATION_BATCH RE_GUEST_MEMORY_MAX_REGIONS
#define RE_GUEST_ELF32_MAX_RELOCATIONS 4096u

typedef struct {
    void *host_buffer;
    size_t capacity;
} REGUESTELF32LOADBUFFER;

typedef struct {
    const uint8_t *elf_bytes;
    size_t elf_size;
    uint32_t load_bias;
    const REGUESTELF32LOADBUFFER *buffers;
    size_t buffer_count;
    /* Relocations must already be decoded and symbol-resolved. Their target
     * and symbol_value fields must already be guest virtual addresses after
     * load-bias/provider selection. RELATIVE entries receive this load_bias. */
    const REGUESTRELOCATION *relocations;
    size_t relocation_count;
} REGUESTELF32LOADREQUEST;

typedef struct {
    uint32_t entry_point;
    uint8_t entry_valid;
    uint8_t thumb_entry;
    uint8_t load_count;
    REGUESTELF32DYNAMIC dynamic;
} REGUESTELF32LOADRESULT;

typedef enum {
    RE_GUEST_ELF32_LOAD_OK = 0,
    RE_GUEST_ELF32_LOAD_E_ARGUMENT = -700,
    RE_GUEST_ELF32_LOAD_E_IMAGE = -701,
    RE_GUEST_ELF32_LOAD_E_DYNAMIC = -702,
    RE_GUEST_ELF32_LOAD_E_BIAS = -703,
    RE_GUEST_ELF32_LOAD_E_ENTRY = -704,
    RE_GUEST_ELF32_LOAD_E_SEGMENT = -705,
    RE_GUEST_ELF32_LOAD_E_BUFFER = -706,
    RE_GUEST_ELF32_LOAD_E_FLAGS = -707,
    RE_GUEST_ELF32_LOAD_E_MAP = -708,
    RE_GUEST_ELF32_LOAD_E_MATERIALIZE = -709,
    RE_GUEST_ELF32_LOAD_E_RELOCATION = -710,
    RE_GUEST_ELF32_LOAD_E_PERMISSION = -711,
    RE_GUEST_ELF32_LOAD_E_ROLLBACK = -712
} REGUESTELF32LOADStatus;

/*
 * Load one caller-selected ELF32 EM_ARM image into caller-owned buffers.
 * The operation parses PT_LOAD/PT_DYNAMIC, maps all segments temporarily
 * writable, copies file bytes and zeroes BSS, applies caller-prepared
 * relocations, and atomically finalizes guest permissions (W+X rejected).
 *
 * On any post-map failure, every mapping created by this call is removed.
 * Caller buffers are borrowed and may contain partially materialized bytes
 * after failure; discard or reinitialize them before retrying.
 *
 * This is not a dependency-searching dynamic linker: the caller must supply
 * the selected image, provider set, and fully decoded/resolved relocations.
 * It does not open files, allocate host memory, emulate Bionic namespaces,
 * execute constructors, or start the guest CPU.
 */
REGUESTELF32LOADStatus re_guest_elf32_load_image(
    REGUESTMEMORY *memory,
    const REGUESTELF32LOADREQUEST *request,
    REGUESTELF32LOADRESULT *result,
    size_t *failed_segment,
    size_t *failed_relocation);

#ifdef __cplusplus
}
#endif
#endif
