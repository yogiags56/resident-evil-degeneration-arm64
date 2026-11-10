#include "re_guest_elf32_init_exec.h"
#include <string.h>

static REGUESTIMPORTStatus init_return_handler(
    const S3EAAPCS32Frame *frame,const REGUESTMEMBUS *bus,void *user,
    REGUESTIMPORTResult *result) {
    REGUESTELF32INITEXECSTATE *state=(REGUESTELF32INITEXECSTATE *)user;
    (void)frame;(void)bus;
    if(!state||!result||!state->active)return RE_IMPORT_E_HANDLER;
    state->completed=1u;
    state->return_count++;
    result->r0=0u;result->r1=0u;
    return RE_IMPORT_OK;
}
REGUESTIMPORTStatus re_guest_elf32_init_bind_return(
    REGUESTIMPORTTable *imports,REGUESTELF32INITEXECSTATE *state,
    uint32_t *thumb_trampoline) {
    if(!state)return RE_IMPORT_E_ARGUMENT;
    state->active=0u;state->completed=0u;state->return_count=0u;
    return re_guest_import_bind_mapped(imports,
        RE_GUEST_ELF32_INIT_RETURN_SYMBOL,init_return_handler,state,
        thumb_trampoline);
}
REGUESTELF32INITEXECStatus re_guest_elf32_execute_init_sequence(
    REGUESTMEMORY *memory,REGUESTMEMBUS *bus,REGUESTIMPORTTable *imports,
    const REGUESTELF32INITCALL *calls,size_t call_count,uint32_t stack_top,
    uint32_t unresolved_pc,uint32_t return_trampoline,uint64_t step_budget,
    REGUESTELF32INITEXECSTATE *state,REGUESTELF32INITEXECRESULT *result) {
    REGUESTELF32INITEXECRESULT pending;
    void *stack_host=NULL;
    if(result)memset(result,0,sizeof(*result));
    memset(&pending,0,sizeof(pending));
    if(!memory||!bus||bus->memory!=memory||!imports||
       (!calls&&call_count)||!state||!result||!step_budget||
       (stack_top&7u)||stack_top<256u||
       !re_guest_import_is_trampoline_pc(imports,return_trampoline&~1u,1)||
       return_trampoline==unresolved_pc)
        return RE_GUEST_ELF32_INIT_EXEC_E_ARGUMENT;
    if(re_guest_memory_resolve(memory,stack_top-256u,256u,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,&stack_host)!=RE_GUEST_MEM_OK)
        return RE_GUEST_ELF32_INIT_EXEC_E_STACK;
    (void)stack_host;
    for(size_t i=0;i<call_count;i++) {
        REGUESTRuntime runtime;
        REGUESTRuntimeStatus rs=RE_RUNTIME_OK;
        state->active=1u;state->completed=0u;
        re_guest_runtime_init_mapped(&runtime,bus,imports,unresolved_pc,
            calls[i].address&~1u,calls[i].thumb);
        runtime.cpu.r[13]=stack_top;
        runtime.cpu.r[14]=return_trampoline;
        for(uint64_t step=0;step<step_budget;step++) {
            rs=re_guest_runtime_step(&runtime);
            pending.total_steps++;
            if(state->completed)break;
            if(rs!=RE_RUNTIME_OK)break;
        }
        state->active=0u;
        if(!state->completed) {
            pending.failed_call=i;
            pending.fault_pc=runtime.fault_pc;
            pending.runtime_status=rs;
            pending.cpu_status=runtime.last_cpu_status;
            pending.import_status=runtime.last_import_status;
            *result=pending;
            return rs==RE_RUNTIME_OK?RE_GUEST_ELF32_INIT_EXEC_E_BUDGET:
                RE_GUEST_ELF32_INIT_EXEC_E_RUNTIME;
        }
        state->completed=0u;
        pending.completed_calls++;
    }
    pending.runtime_status=RE_RUNTIME_OK;
    pending.cpu_status=RE_CPU_OK;
    pending.import_status=RE_IMPORT_OK;
    pending.failed_call=call_count;
    *result=pending;
    return RE_GUEST_ELF32_INIT_EXEC_OK;
}
