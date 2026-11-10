#ifndef RE_GUEST_ELF32_MULTI_LOADER_H
#define RE_GUEST_ELF32_MULTI_LOADER_H
#include "re_guest_elf32_loader.h"
#include "re_guest_elf32_module_graph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    REGUESTELF32MODULEDESC module;
    REGUESTELF32LOADREQUEST load;
} REGUESTELF32MODULELOADREQUEST;
typedef struct {
    REGUESTELF32MODULEGRAPH graph;
    REGUESTELF32LOADRESULT results[RE_GUEST_ELF32_MAX_MODULES];
    size_t loaded_count;
    size_t failed_module_index;
    int inner_status;
} REGUESTELF32MULTILOADRESULT;
typedef enum {
    RE_GUEST_ELF32_MULTI_LOAD_OK = 0,
    RE_GUEST_ELF32_MULTI_LOAD_E_ARGUMENT = -740,
    RE_GUEST_ELF32_MULTI_LOAD_E_GRAPH = -741,
    RE_GUEST_ELF32_MULTI_LOAD_E_RELOCATION_TARGET = -742,
    RE_GUEST_ELF32_MULTI_LOAD_E_IMAGE = -743,
    RE_GUEST_ELF32_MULTI_LOAD_E_LOAD = -744,
    RE_GUEST_ELF32_MULTI_LOAD_E_ROLLBACK = -745
} REGUESTELF32MULTILOADStatus;
/* Transactionally load a root-reachable module closure in dependency-first
 * order. Each request must contain already-resolved relocation records and
 * caller-owned segment buffers. All new mappings are removed if any module
 * fails. Borrowed buffers touched before failure must be discarded/reset.
 * Relocation targets are restricted to the owning module's PT_LOAD segments,
 * so pre-existing mappings cannot be modified by this transaction.
 */
REGUESTELF32MULTILOADStatus re_guest_elf32_load_module_set(
    REGUESTMEMORY *memory, const REGUESTELF32MODULELOADREQUEST *requests,
    size_t request_count, const char *root_soname,
    REGUESTELF32MULTILOADRESULT *result);
#ifdef __cplusplus
}
#endif
#endif
