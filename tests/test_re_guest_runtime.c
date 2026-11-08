#include "re_guest_runtime.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static REGUESTIMPORTStatus add_handler(const S3EAAPCS32Frame *f,
    const REGUESTMEM *m, void *u, REGUESTIMPORTResult *r) {
    uint32_t fifth=0; (void)m;(void)u;
    if (s3e_aapcs32_arg_word(f,4,&fifth)!=S3E_AAPCS32_OK) return RE_IMPORT_E_HANDLER;
    r->r0=f->r[0]+f->r[1]+fifth; r->r1=0;
    return RE_IMPORT_OK;
}
int main(void) {
    uint8_t bytes[256]={0};
    REGUESTMEM mem={bytes,sizeof(bytes),0x50000000u};
    REGUESTIMPORTTable imports;
    REGUESTRuntime rt;
    uint32_t tramp=0;
    uint64_t done=0;
    /* Thumb MOVS r2,#9 at guest 0x50000000. */
    bytes[0]=0x09; bytes[1]=0x22;
    /* stack argument 5 at guest SP=0x50000080 */
    bytes[0x80]=7;
    assert(re_guest_import_table_init(&imports,0x51000000u)==RE_IMPORT_OK);
    assert(re_guest_import_bind(&imports,3,add_handler,0,&tramp)==RE_IMPORT_OK);
    re_guest_runtime_init(&rt,&mem,&imports,0x4A000070u,0x50000000u,1);
    assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
    assert(rt.cpu.r[2]==9 && rt.cpu_instructions==1 && rt.dispatched_imports==0);
    rt.cpu.r[15]=tramp&~1u; rt.cpu.thumb=1;
    rt.cpu.r[14]=0x50000021u; rt.cpu.r[13]=0x50000080u;
    rt.cpu.r[0]=100; rt.cpu.r[1]=200;
    assert(re_guest_runtime_step(&rt)==RE_RUNTIME_OK);
    assert(rt.cpu.r[0]==307 && rt.cpu.r[15]==0x50000020u);
    assert(rt.dispatched_imports==1 && rt.cpu_instructions==1 && rt.cpu.steps==2);
    rt.cpu.r[15]=0x4A000070u; rt.cpu.thumb=1;
    assert(re_guest_runtime_step(&rt)==RE_RUNTIME_E_UNRESOLVED_IMPORT);
    assert(rt.fault_valid && rt.fault_pc==0x4A000070u &&
           rt.fault_thumb==1u &&
           rt.last_stop_status==RE_RUNTIME_E_UNRESOLVED_IMPORT &&
           rt.last_import_status==RE_IMPORT_E_UNBOUND);
    re_guest_runtime_init(&rt,&mem,&imports,0x4A000070u,0x50000000u,1);
    assert(re_guest_runtime_run(&rt,1,&done)==RE_RUNTIME_OK && done==1);
    assert(re_guest_runtime_run(&rt,1,&done)==RE_RUNTIME_OK && done==1);
    assert(rt.cpu_instructions==2);
    /* Import dispatch failure preserves the trampoline PC and ABI error. */
    rt.cpu.r[15]=tramp&~1u; rt.cpu.thumb=1;
    rt.cpu.r[13]=0x50000081u; rt.cpu.r[14]=0x50000021u;
    assert(re_guest_runtime_step(&rt)==RE_RUNTIME_E_IMPORT);
    assert(rt.fault_valid && rt.fault_pc==(tramp&~1u) &&
           rt.last_import_status==RE_IMPORT_E_MEMORY &&
           rt.last_stop_status==RE_RUNTIME_E_IMPORT);
    /* Guest instruction fetch fault is distinguishable from import failure. */
    re_guest_runtime_init(&rt,&mem,&imports,0x4A000070u,0x4FFFFFFEu,1);
    assert(re_guest_runtime_step(&rt)==RE_RUNTIME_E_CPU);
    assert(rt.fault_valid && rt.fault_pc==0x4FFFFFFEu &&
           rt.last_cpu_status==RE_CPU_E_MEMORY &&
           rt.last_stop_status==RE_RUNTIME_E_CPU);
    puts("Guest runtime coordinator tests: PASS (expanded fault diagnostics)");
    return 0;
}
