#ifndef RE_GUEST_ELF32_INIT_EXEC_H
#define RE_GUEST_ELF32_INIT_EXEC_H
#include "re_guest_elf32_init.h"
#include "re_guest_runtime.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RE_GUEST_ELF32_INIT_RETURN_SYMBOL 0xfffffff0u
typedef struct {
    uint8_t active;
    uint8_t completed;
    uint64_t return_count;
} REGUESTELF32INITEXECSTATE;
typedef struct {
    size_t completed_calls;
    size_t failed_call;
    uint64_t total_steps;
    uint32_t fault_pc;
    REGUESTRuntimeStatus runtime_status;
    REGUESTCPUStatus cpu_status;
    REGUESTIMPORTStatus import_status;
} REGUESTELF32INITEXECRESULT;
typedef enum {
    RE_GUEST_ELF32_INIT_EXEC_OK=0,
    RE_GUEST_ELF32_INIT_EXEC_E_ARGUMENT=-790,
    RE_GUEST_ELF32_INIT_EXEC_E_BIND=-791,
    RE_GUEST_ELF32_INIT_EXEC_E_STACK=-792,
    RE_GUEST_ELF32_INIT_EXEC_E_RUNTIME=-793,
    RE_GUEST_ELF32_INIT_EXEC_E_BUDGET=-794
} REGUESTELF32INITEXECStatus;
/* Bind once and retain state for at least as long as the import table. */
REGUESTIMPORTStatus re_guest_elf32_init_bind_return(
    REGUESTIMPORTTable *imports,REGUESTELF32INITEXECSTATE *state,
    uint32_t *thumb_trampoline);
/* Execute validated constructor calls serially with a fresh bounded runtime
 * per call. stack_top is the initial AAPCS32 SP and must be 8-byte aligned.
 * No constructor may run unbounded: max_steps_per_call is mandatory.
 */
REGUESTELF32INITEXECStatus re_guest_elf32_execute_init_sequence(
    REGUESTMEMORY *memory,REGUESTMEMBUS *bus,REGUESTIMPORTTable *imports,
    const REGUESTELF32INITCALL *calls,size_t call_count,uint32_t stack_top,
    uint32_t unresolved_placeholder_pc,uint32_t return_trampoline_thumb,
    uint64_t max_steps_per_call,REGUESTELF32INITEXECSTATE *state,
    REGUESTELF32INITEXECRESULT *result);
#ifdef __cplusplus
}
#endif
#endif
