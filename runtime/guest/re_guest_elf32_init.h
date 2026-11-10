#ifndef RE_GUEST_ELF32_INIT_H
#define RE_GUEST_ELF32_INIT_H
#include "re_guest_elf32_pipeline.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RE_GUEST_ELF32_MAX_INIT_CALLS 4096u
typedef enum {
    RE_GUEST_ELF32_INIT_KIND_DT_INIT=1,
    RE_GUEST_ELF32_INIT_KIND_INIT_ARRAY=2
} REGUESTELF32INITKIND;
typedef struct {
    uint32_t address; /* AArch32 function pointer; bit 0 selects Thumb state. */
    size_t module_index;
    uint8_t kind;
    uint8_t thumb;
} REGUESTELF32INITCALL;
typedef enum {
    RE_GUEST_ELF32_INIT_OK=0,
    RE_GUEST_ELF32_INIT_E_ARGUMENT=-780,
    RE_GUEST_ELF32_INIT_E_CAPACITY=-781,
    RE_GUEST_ELF32_INIT_E_RANGE=-782,
    RE_GUEST_ELF32_INIT_E_MEMORY=-783,
    RE_GUEST_ELF32_INIT_E_PERMISSION=-784,
    RE_GUEST_ELF32_INIT_E_ENTRY=-785
} REGUESTELF32INITStatus;
/* Build, but do not execute, the constructor sequence from an already-loaded
 * pipeline. Dependency-first module order is preserved. Within each module,
 * DT_INIT precedes DT_INIT_ARRAY. Array entries are read after relocation.
 * Output remains unchanged on failure.
 */
REGUESTELF32INITStatus re_guest_elf32_build_init_sequence(
    const REGUESTMEMORY *memory,const REGUESTELF32PIPELINEMODULE *modules,
    size_t module_count,const REGUESTELF32PIPELINERESULT *pipeline,
    REGUESTELF32INITCALL *calls,size_t call_capacity,size_t *call_count,
    size_t *failed_module,size_t *failed_call);
#ifdef __cplusplus
}
#endif
#endif
