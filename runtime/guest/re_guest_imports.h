#ifndef RE_GUEST_IMPORTS_H
#define RE_GUEST_IMPORTS_H
#include <stddef.h>
#include <stdint.h>
#include "re_guest_cpu.h"
#include "s3e_aapcs32.h"
#include "re_guest_memory_bus.h"

#define RE_IMPORT_MAPPED_STACK_BYTES 256u

#ifdef __cplusplus
extern "C" {
#endif

#define RE_IMPORT_MAX_BINDINGS 256u

typedef enum {
    RE_IMPORT_OK = 0,
    RE_IMPORT_E_ARGUMENT = -1,
    RE_IMPORT_E_MEMORY = -2,
    RE_IMPORT_E_CAPACITY = -3,
    RE_IMPORT_E_DUPLICATE = -4,
    RE_IMPORT_E_UNBOUND = -5,
    RE_IMPORT_E_HANDLER = -6,
    RE_IMPORT_E_BAD_RETURN = -7
} REGUESTIMPORTStatus;

typedef struct {
    uint32_t r0;
    uint32_t r1;
} REGUESTIMPORTResult;

typedef REGUESTIMPORTStatus (*REGUESTIMPORTHandler)(
    const S3EAAPCS32Frame *frame,
    const REGUESTMEM *guest_memory,
    void *user,
    REGUESTIMPORTResult *result);

typedef REGUESTIMPORTStatus (*REGUESTIMPORTMappedHandler)(
    const S3EAAPCS32Frame *frame,
    const REGUESTMEMBUS *guest_bus,
    void *user,
    REGUESTIMPORTResult *result);

typedef struct {
    uint32_t symbol_index;
    uint32_t trampoline_address; /* Address without Thumb state bit. */
    REGUESTIMPORTHandler handler;
    REGUESTIMPORTMappedHandler mapped_handler;
    void *user;
} REGUESTIMPORTBinding;

typedef struct {
    REGUESTIMPORTBinding entries[RE_IMPORT_MAX_BINDINGS];
    size_t count;
    uint32_t trampoline_base;
} REGUESTIMPORTTable;

/* Reserve a virtual Thumb trampoline range. Trampolines are intercepted by
 * re_guest_import_dispatch before instruction fetch; no guest code bytes exist.
 */
REGUESTIMPORTStatus re_guest_import_table_init(REGUESTIMPORTTable *table,
                                                uint32_t trampoline_base);

/* Register a handler for a symbol index. Duplicate symbols are rejected. */
REGUESTIMPORTStatus re_guest_import_bind_mapped(REGUESTIMPORTTable *table,
                                          uint32_t symbol_index,
                                          REGUESTIMPORTMappedHandler handler,
                                          void *user,
                                          uint32_t *thumb_pointer);

REGUESTIMPORTStatus re_guest_import_bind(REGUESTIMPORTTable *table,
                                          uint32_t symbol_index,
                                          REGUESTIMPORTHandler handler,
                                          void *user,
                                          uint32_t *thumb_pointer);

/* Patch a guest import pointer slot with its symbol's Thumb trampoline pointer.
 * slot_offset is relative to guest_memory->guest_base.
 */
REGUESTIMPORTStatus re_guest_import_patch_slot(const REGUESTIMPORTTable *table,
                                                uint32_t symbol_index,
                                                REGUESTMEM *guest_memory,
                                                uint32_t slot_address);

/* If cpu->r[15] is a registered trampoline address, marshal AAPCS32 args,
 * call its native handler, set r0/r1, and return through guest LR.
 * Returns RE_IMPORT_E_UNBOUND for unregistered virtual trampoline PCs.
 */
int re_guest_import_is_trampoline_pc(const REGUESTIMPORTTable *table,
                                     uint32_t pc, int thumb_state);

REGUESTIMPORTStatus re_guest_import_patch_slot_mapped(
    const REGUESTIMPORTTable *table, uint32_t symbol_index,
    REGUESTMEMBUS *guest_bus, uint32_t slot_address);

REGUESTIMPORTStatus re_guest_import_dispatch_mapped(
    REGUESTIMPORTTable *table, REGUESTCPU *cpu, const REGUESTMEMBUS *guest_bus);

REGUESTIMPORTStatus re_guest_import_dispatch(REGUESTIMPORTTable *table,
                                              REGUESTCPU *cpu,
                                              const REGUESTMEM *guest_memory);

#ifdef __cplusplus
}
#endif
#endif
