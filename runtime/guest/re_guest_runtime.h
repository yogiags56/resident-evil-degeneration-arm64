#ifndef RE_GUEST_RUNTIME_H
#define RE_GUEST_RUNTIME_H
#include <stdint.h>
#include "re_guest_cpu.h"
#include "re_guest_imports.h"

typedef enum {
    RE_RUNTIME_OK = 0,
    RE_RUNTIME_HALTED = 1,
    RE_RUNTIME_E_ARGUMENT = -100,
    RE_RUNTIME_E_CPU = -101,
    RE_RUNTIME_E_IMPORT = -102,
    RE_RUNTIME_E_UNRESOLVED_IMPORT = -103
} REGUESTRuntimeStatus;

typedef struct {
    REGUESTCPU cpu;
    REGUESTMEM *memory;
    REGUESTIMPORTTable *imports;
    uint32_t unresolved_placeholder_pc;
    uint64_t dispatched_imports;
    uint64_t cpu_instructions;
    /* Last-stop diagnostics; guest addresses remain 32-bit values. */
    uint32_t fault_pc;
    uint8_t fault_thumb;
    uint8_t fault_valid;
    REGUESTCPUStatus last_cpu_status;
    REGUESTIMPORTStatus last_import_status;
    REGUESTRuntimeStatus last_stop_status;
} REGUESTRuntime;

void re_guest_runtime_init(REGUESTRuntime *runtime,
                           REGUESTMEM *memory,
                           REGUESTIMPORTTable *imports,
                           uint32_t unresolved_placeholder_pc,
                           uint32_t entry,
                           int thumb_state);

/* Dispatch imports before instruction fetch; otherwise execute one guest instruction. */
REGUESTRuntimeStatus re_guest_runtime_step(REGUESTRuntime *runtime);

/* Run until error/halt or max_steps. Import dispatches and CPU instructions both
 * consume one runtime step. cpu_instructions and dispatched_imports are separate.
 */
REGUESTRuntimeStatus re_guest_runtime_run(REGUESTRuntime *runtime,
                                           uint64_t max_steps,
                                           uint64_t *steps_done);
#endif
