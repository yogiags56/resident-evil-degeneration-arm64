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
    re_guest_cpu_reset(&r->cpu,entry,thumb_state);
}

REGUESTRuntimeStatus re_guest_runtime_step(REGUESTRuntime *r) {
    REGUESTIMPORTStatus is;
    REGUESTCPUStatus cs;
    uint32_t pc;
    if (!r || !r->memory || !r->imports) return RE_RUNTIME_E_ARGUMENT;
    if (r->cpu.halted) return RE_RUNTIME_HALTED;
    pc=r->cpu.r[15];
    if (r->cpu.thumb && pc==(r->unresolved_placeholder_pc)) {
        return RE_RUNTIME_E_UNRESOLVED_IMPORT;
    }
    if (re_guest_import_is_trampoline_pc(r->imports,pc,r->cpu.thumb)) {
        is=re_guest_import_dispatch(r->imports,&r->cpu,r->memory);
        if (is!=RE_IMPORT_OK) return RE_RUNTIME_E_IMPORT;
        r->dispatched_imports++;
        r->cpu.steps++;
        return RE_RUNTIME_OK;
    }
    cs=re_guest_cpu_step(&r->cpu,r->memory);
    if (cs==RE_CPU_OK) {
        r->cpu_instructions++;
        return RE_RUNTIME_OK;
    }
    if (cs==RE_CPU_HALTED) return RE_RUNTIME_HALTED;
    return RE_RUNTIME_E_CPU;
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
