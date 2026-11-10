#ifndef RE_GUEST_ELF32_BOOT_H
#define RE_GUEST_ELF32_BOOT_H
#include "re_guest_elf32_pipeline.h"
#include "re_guest_runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    size_t root_module_index;
    uint32_t entry_point;
    uint32_t stack_pointer;
    uint8_t thumb_state;
} REGUESTELF32BOOTINFO;
typedef enum {
    RE_GUEST_ELF32_BOOT_OK=0,
    RE_GUEST_ELF32_BOOT_E_ARGUMENT=-800,
    RE_GUEST_ELF32_BOOT_E_ROOT=-801,
    RE_GUEST_ELF32_BOOT_E_ENTRY=-802,
    RE_GUEST_ELF32_BOOT_E_STACK=-803
} REGUESTELF32BOOTStatus;
/* Initialize, but do not run, the root executable after the pipeline and
 * constructor sequence have completed. stack_top must have 256 readable/
 * writable bytes below it and be 8-byte aligned.
 */
REGUESTELF32BOOTStatus re_guest_elf32_start_root_runtime(
    REGUESTMEMBUS *bus,REGUESTIMPORTTable *imports,
    const REGUESTELF32PIPELINEMODULE *modules,size_t module_count,
    const REGUESTELF32PIPELINERESULT *pipeline,uint32_t stack_top,
    uint32_t unresolved_placeholder_pc,REGUESTRuntime *runtime,
    REGUESTELF32BOOTINFO *info);
#ifdef __cplusplus
}
#endif
#endif
