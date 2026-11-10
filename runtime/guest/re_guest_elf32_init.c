#include "re_guest_elf32_init.h"
#include <string.h>

static uint32_t read_u32(const uint8_t *p,int little) {
    if(little)return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|
        ((uint32_t)p[2]<<8)|(uint32_t)p[3];
}
static REGUESTELF32INITStatus append_call(
    const REGUESTMEMORY *memory,uint32_t address,size_t module_index,
    REGUESTELF32INITKIND kind,REGUESTELF32INITCALL *tmp,size_t *count,
    size_t *failed_call) {
    void *host=NULL;
    uint32_t code_address=address&~1u;
    size_t width=(address&1u)?2u:4u;
    REGUESTMEMStatus ms;
    if(!address||address==UINT32_MAX)return RE_GUEST_ELF32_INIT_OK;
    ms=re_guest_memory_resolve(memory,code_address,width,RE_GUEST_MEM_EXEC,&host);
    if(ms!=RE_GUEST_MEM_OK)return RE_GUEST_ELF32_INIT_E_ENTRY;
    if(*count>=RE_GUEST_ELF32_MAX_INIT_CALLS) {
        if(failed_call)*failed_call=*count;
        return RE_GUEST_ELF32_INIT_E_CAPACITY;
    }
    tmp[*count].address=address;
    tmp[*count].module_index=module_index;
    tmp[*count].kind=(uint8_t)kind;
    tmp[*count].thumb=(uint8_t)(address&1u);
    (*count)++;
    (void)host;
    return RE_GUEST_ELF32_INIT_OK;
}
REGUESTELF32INITStatus re_guest_elf32_build_init_sequence(
    const REGUESTMEMORY *memory,const REGUESTELF32PIPELINEMODULE *modules,
    size_t module_count,const REGUESTELF32PIPELINERESULT *pipeline,
    REGUESTELF32INITCALL *calls,size_t capacity,size_t *call_count,
    size_t *failed_module,size_t *failed_call) {
    REGUESTELF32INITCALL pending[RE_GUEST_ELF32_MAX_INIT_CALLS];
    size_t count=0;
    if(call_count)*call_count=0;
    if(failed_module)*failed_module=0;
    if(failed_call)*failed_call=0;
    if(!memory||!modules||!module_count||
       module_count>RE_GUEST_ELF32_MAX_MODULES||!pipeline||
       pipeline->module_count!=module_count||!call_count||
       (!calls&&capacity)||pipeline->graph.order_count>module_count||
       !pipeline->graph.complete)
        return RE_GUEST_ELF32_INIT_E_ARGUMENT;
    memset(pending,0,sizeof(pending));
    for(size_t order=0;order<pipeline->graph.order_count;order++) {
        size_t i=pipeline->graph.order[order];
        const REGUESTELF32DYNAMIC *d;
        uint32_t bias;
        size_t failed=0;
        if(i>=module_count||!pipeline->graph.reachable[i]) {
            if(failed_module)*failed_module=i;
            return RE_GUEST_ELF32_INIT_E_ARGUMENT;
        }
        d=&pipeline->metadata[i].dynamic;
        bias=modules[i].load_bias;
        if(d->has_init&&d->init_address) {
            uint64_t address=(uint64_t)d->init_address+bias;
            if(address>UINT32_MAX) {
                if(failed_module)*failed_module=i;
                return RE_GUEST_ELF32_INIT_E_RANGE;
            }
            REGUESTELF32INITStatus s=append_call(memory,(uint32_t)address,i,
                RE_GUEST_ELF32_INIT_KIND_DT_INIT,pending,&count,&failed);
            if(s!=RE_GUEST_ELF32_INIT_OK) {
                if(failed_module)*failed_module=i;
                if(failed_call)*failed_call=failed;
                return s;
            }
        }
        if(d->has_init_array&&d->has_init_array_size&&d->init_array_size) {
            uint64_t array_address=(uint64_t)d->init_array_address+bias;
            void *host=NULL;
            if((d->init_array_size&3u)!=0u||array_address>UINT32_MAX||
               array_address+d->init_array_size>0x100000000ULL) {
                if(failed_module)*failed_module=i;
                return RE_GUEST_ELF32_INIT_E_RANGE;
            }
            REGUESTMEMStatus ms=re_guest_memory_resolve(memory,
                (uint32_t)array_address,d->init_array_size,
                RE_GUEST_MEM_READ,&host);
            if(ms!=RE_GUEST_MEM_OK) {
                if(failed_module)*failed_module=i;
                return ms==RE_GUEST_MEM_E_PERMISSION?
                    RE_GUEST_ELF32_INIT_E_PERMISSION:RE_GUEST_ELF32_INIT_E_MEMORY;
            }
            for(size_t n=0;n<d->init_array_size/4u;n++) {
                uint32_t address=read_u32((const uint8_t *)host+n*4u,
                    pipeline->metadata[i].image.little_endian);
                REGUESTELF32INITStatus s=append_call(memory,address,i,
                    RE_GUEST_ELF32_INIT_KIND_INIT_ARRAY,pending,&count,&failed);
                if(s!=RE_GUEST_ELF32_INIT_OK) {
                    if(failed_module)*failed_module=i;
                    if(failed_call)*failed_call=n;
                    return s;
                }
            }
        }
    }
    if(count>capacity)return RE_GUEST_ELF32_INIT_E_CAPACITY;
    if(count)memcpy(calls,pending,count*sizeof(*calls));
    *call_count=count;
    if(failed_module)*failed_module=module_count;
    return RE_GUEST_ELF32_INIT_OK;
}
