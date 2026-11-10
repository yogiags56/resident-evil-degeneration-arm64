#include "re_guest_elf32_boot.h"
#include <string.h>
REGUESTELF32BOOTStatus re_guest_elf32_start_root_runtime(
    REGUESTMEMBUS *bus,REGUESTIMPORTTable *imports,
    const REGUESTELF32PIPELINEMODULE *modules,size_t module_count,
    const REGUESTELF32PIPELINERESULT *pipeline,uint32_t stack_top,
    uint32_t unresolved_pc,REGUESTRuntime *runtime,
    REGUESTELF32BOOTINFO *info) {
    size_t root,load_index=module_count;
    uint64_t expected;
    void *host=NULL;
    uint32_t entry;
    uint8_t thumb;
    if(info)memset(info,0,sizeof(*info));
    if(!bus||!bus->memory||!imports||!modules||!module_count||
       !pipeline||pipeline->module_count!=module_count||!runtime||!info||
       pipeline->root_module_index>=module_count||!pipeline->graph.complete||
       !pipeline->graph.reachable[pipeline->root_module_index]||
       pipeline->graph.order_count!=pipeline->loaded.module_count||
       (stack_top&7u)||stack_top<256u)
        return RE_GUEST_ELF32_BOOT_E_ARGUMENT;
    root=pipeline->root_module_index;
    for(size_t k=0;k<pipeline->graph.order_count;k++) {
        if(pipeline->graph.order[k]==root){load_index=k;break;}
    }
    if(load_index==module_count)return RE_GUEST_ELF32_BOOT_E_ROOT;
    const REGUESTELF32LOADRESULT *loaded=&pipeline->loaded.modules[load_index];
    if(!loaded->entry_valid)return RE_GUEST_ELF32_BOOT_E_ENTRY;
    expected=(uint64_t)pipeline->metadata[root].image.entry+
        modules[root].load_bias;
    if(expected>UINT32_MAX||loaded->entry_point!=(uint32_t)expected||
       loaded->thumb_entry!=(uint8_t)(expected&1u))
        return RE_GUEST_ELF32_BOOT_E_ENTRY;
    entry=loaded->entry_point;
    thumb=loaded->thumb_entry;
    if(re_guest_memory_resolve(bus->memory,entry&~1u,thumb?2u:4u,
        RE_GUEST_MEM_EXEC,&host)!=RE_GUEST_MEM_OK)
        return RE_GUEST_ELF32_BOOT_E_ENTRY;
    if(re_guest_memory_resolve(bus->memory,stack_top-256u,256u,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE,&host)!=RE_GUEST_MEM_OK)
        return RE_GUEST_ELF32_BOOT_E_STACK;
    re_guest_runtime_init_mapped(runtime,bus,imports,unresolved_pc,
        entry&~1u,thumb);
    runtime->cpu.r[13]=stack_top;
    runtime->cpu.r[14]=unresolved_pc|1u;
    info->root_module_index=root;
    info->entry_point=entry;
    info->stack_pointer=stack_top;
    info->thumb_state=thumb;
    return RE_GUEST_ELF32_BOOT_OK;
}
