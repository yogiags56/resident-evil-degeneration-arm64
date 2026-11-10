#include "re_guest_elf32_multi_loader.h"
#include <string.h>
static int relocation_targets_owned(const REGUESTELF32LOADREQUEST *r,
    const REGUESTELF32IMAGE *image) {
    for(size_t n=0;n<r->relocation_count;n++) {
        uint64_t target=(uint64_t)r->relocations[n].target;
        int found=0;
        for(size_t p=0;p<image->program_header_count;p++) {
            const REGUESTELF32PROGRAMHEADER *ph=&image->program_headers[p];
            uint64_t begin,end;
            if(ph->type!=RE_GUEST_ELF32_PT_LOAD||!ph->memory_size)continue;
            begin=(uint64_t)ph->virtual_address+r->load_bias;
            end=begin+ph->memory_size;
            if(target>=begin&&target+4u<=end&&target+4u<=0x100000000ULL) {
                found=1;break;
            }
        }
        if(!found)return 0;
    }
    return 1;
}
REGUESTELF32MULTILOADStatus re_guest_elf32_load_module_set(
    REGUESTMEMORY *memory,const REGUESTELF32MODULELOADREQUEST *requests,
    size_t count,const char *root,REGUESTELF32MULTILOADRESULT *result) {
    REGUESTELF32MULTILOADRESULT pending;
    REGUESTMEMORY before;
    REGUESTELF32MODULEDESC descriptions[RE_GUEST_ELF32_MAX_MODULES];
    size_t failed=0;
    if(result)memset(result,0,sizeof(*result));
    memset(&pending,0,sizeof(pending));
    if(!memory||!requests||!count||count>RE_GUEST_ELF32_MAX_MODULES||
       !root||!result)return RE_GUEST_ELF32_MULTI_LOAD_E_ARGUMENT;
    for(size_t i=0;i<count;i++)descriptions[i]=requests[i].module;
    REGUESTELF32MODULEGRAPHStatus gs=re_guest_elf32_build_module_graph(
        descriptions,count,root,&pending.graph,&failed);
    if(gs!=RE_GUEST_ELF32_MODULE_GRAPH_OK) {
        pending.failed_module_index=failed;*result=pending;
        return RE_GUEST_ELF32_MULTI_LOAD_E_GRAPH;
    }
    /* Preflight every reachable ELF and ensure relocation writes stay inside
       that module's own PT_LOAD image before changing guest memory. */
    for(size_t n=0;n<pending.graph.order_count;n++) {
        size_t i=pending.graph.order[n];
        REGUESTELF32IMAGE image;
        const REGUESTELF32LOADREQUEST *load=&requests[i].load;
        if(!load->elf_bytes||(!load->buffers&&load->buffer_count)||
           (!load->relocations&&load->relocation_count)) {
            pending.failed_module_index=i;*result=pending;
            return RE_GUEST_ELF32_MULTI_LOAD_E_ARGUMENT;
        }
        REGUESTELF32Status is=re_guest_elf32_parse_image(
            load->elf_bytes,load->elf_size,&image,&failed);
        if(is!=RE_GUEST_ELF32_OK) {
            pending.failed_module_index=i;pending.inner_status=(int)is;
            *result=pending;return RE_GUEST_ELF32_MULTI_LOAD_E_IMAGE;
        }
        if(!relocation_targets_owned(load,&image)) {
            pending.failed_module_index=i;*result=pending;
            return RE_GUEST_ELF32_MULTI_LOAD_E_RELOCATION_TARGET;
        }
    }
    before=*memory;
    for(size_t n=0;n<pending.graph.order_count;n++) {
        size_t i=pending.graph.order[n];
        REGUESTELF32LOADStatus ls=re_guest_elf32_load_image(
            memory,&requests[i].load,&pending.results[i],&failed,NULL);
        if(ls!=RE_GUEST_ELF32_LOAD_OK) {
            *memory=before;
            pending.failed_module_index=i;pending.inner_status=(int)ls;
            pending.loaded_count=0;
            memset(pending.results,0,sizeof(pending.results));
            *result=pending;
            return RE_GUEST_ELF32_MULTI_LOAD_E_LOAD;
        }
        pending.loaded_count++;
    }
    *result=pending;
    return RE_GUEST_ELF32_MULTI_LOAD_OK;
}
