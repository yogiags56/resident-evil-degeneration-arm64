#include "re_guest_runtime.h"

void re_guest_runtime_init(REGUESTRuntime *r, REGUESTMEM *m,
                           REGUESTIMPORTTable *imports,
                           uint32_t unresolved_pc,
                           uint32_t entry, int thumb_state) {
    if (!r) return;
    for (size_t i=0;i<sizeof(*r);i++) ((uint8_t *)r)[i]=0;
    r->memory=m;
    r->imports=imports;
    r->unresolved_placeholder_pc=unresolved_pc & ~1u;
    r->last_cpu_status=RE_CPU_OK;
    r->last_import_status=RE_IMPORT_OK;
    r->last_stop_status=RE_RUNTIME_OK;
    re_guest_cpu_reset(&r->cpu,entry,thumb_state);
}

static REGUESTRuntimeStatus stop_runtime(REGUESTRuntime *r,
                                         REGUESTRuntimeStatus status,
                                         uint32_t pc,
                                         REGUESTIMPORTStatus import_status,
                                         REGUESTCPUStatus cpu_status) {
    r->fault_pc=pc;
    r->fault_thumb=r->cpu.thumb;
    r->fault_valid=1u;
    r->last_import_status=import_status;
    r->last_cpu_status=cpu_status;
    r->last_stop_status=status;
    return status;
}

REGUESTRuntimeStatus re_guest_runtime_step(REGUESTRuntime *r) {
    REGUESTIMPORTStatus is;
    REGUESTCPUStatus cs;
    uint32_t pc;
    if (!r || !r->memory || !r->imports) return RE_RUNTIME_E_ARGUMENT;
    if (r->cpu.halted)
        return stop_runtime(r,RE_RUNTIME_HALTED,r->cpu.r[15],
                            RE_IMPORT_OK,RE_CPU_HALTED);
    pc=r->cpu.r[15];
    if (r->cpu.thumb && pc==r->unresolved_placeholder_pc)
        return stop_runtime(r,RE_RUNTIME_E_UNRESOLVED_IMPORT,pc,
                            RE_IMPORT_E_UNBOUND,RE_CPU_OK);
    if (re_guest_import_is_trampoline_pc(r->imports,pc,r->cpu.thumb)) {
        is=re_guest_import_dispatch(r->imports,&r->cpu,r->memory);
        if (is!=RE_IMPORT_OK)
            return stop_runtime(r,RE_RUNTIME_E_IMPORT,pc,is,RE_CPU_OK);
        r->dispatched_imports++;
        r->cpu.steps++;
        r->fault_valid=0u;
        r->last_stop_status=RE_RUNTIME_OK;
        r->last_import_status=RE_IMPORT_OK;
        r->last_cpu_status=RE_CPU_OK;
        return RE_RUNTIME_OK;
    }
    cs=re_guest_cpu_step(&r->cpu,r->memory);
    if (cs==RE_CPU_OK) {
        r->cpu_instructions++;
        r->fault_valid=0u;
        r->last_stop_status=RE_RUNTIME_OK;
        r->last_import_status=RE_IMPORT_OK;
        r->last_cpu_status=RE_CPU_OK;
        return RE_RUNTIME_OK;
    }
    if (cs==RE_CPU_HALTED)
        return stop_runtime(r,RE_RUNTIME_HALTED,pc,RE_IMPORT_OK,cs);
    return stop_runtime(r,RE_RUNTIME_E_CPU,pc,RE_IMPORT_OK,cs);
}

REGUESTRuntimeStatus re_guest_runtime_run(REGUESTRuntime *r,
                                           uint64_t limit,
                                           uint64_t *done) {
    uint64_t n=0;
    if (done) *done=0;
    if (!r || !limit) return RE_RUNTIME_E_ARGUMENT;
    while (n<limit) {
        REGUESTRuntimeStatus s=re_guest_runtime_step(r);
        if (s!=RE_RUNTIME_OK) {
            if (done) *done=n;
            return s;
        }
        n++;
    }
    if (done) *done=n;
    return RE_RUNTIME_OK;
}
