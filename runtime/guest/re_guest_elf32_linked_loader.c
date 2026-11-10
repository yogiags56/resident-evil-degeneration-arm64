#include "re_guest_elf32_linked_loader.h"

#include "re_guest_elf32_image.h"

#include <string.h>

REGUESTELF32LINKEDLOADStatus re_guest_elf32_load_linked_image(
    REGUESTMEMORY *memory,const uint8_t *bytes,size_t size,uint32_t bias,
    const REGUESTELF32LOADBUFFER *buffers,size_t buffer_count,
    const REGUESTSYMBOL *providers,size_t provider_count,
    const REGUESTELF32LINKWORKSPACE *workspace,
    REGUESTELF32LINKEDLOADRESULT *result,
    size_t *failed_index,int *inner_status) {
    REGUESTELF32IMAGE image;
    REGUESTELF32DYNAMIC dynamic;
    REGUESTELF32LINKINFO link;
    REGUESTELF32LOADREQUEST load_request;
    REGUESTELF32LINKEDLOADRESULT pending;
    size_t failed=0,failed_relocation=0;
    if(failed_index)*failed_index=0;
    if(inner_status)*inner_status=0;
    if(result)memset(result,0,sizeof(*result));
    memset(&pending,0,sizeof(pending));
    memset(&dynamic,0,sizeof(dynamic));
    memset(&link,0,sizeof(link));
    if(!memory||!bytes||!result||!workspace||
       (!buffers&&buffer_count)||(!providers&&provider_count)||
       (!workspace->symbols&&workspace->symbol_capacity)||
       (!workspace->decoded_relocations&&workspace->decoded_relocation_capacity)||
       (!workspace->prepared_relocations&&workspace->prepared_relocation_capacity))
        return RE_GUEST_ELF32_LINKED_LOAD_E_ARGUMENT;

    REGUESTELF32Status image_status=re_guest_elf32_parse_image(
        bytes,size,&image,&failed);
    if(image_status!=RE_GUEST_ELF32_OK) {
        if(failed_index)*failed_index=failed;
        if(inner_status)*inner_status=(int)image_status;
        return RE_GUEST_ELF32_LINKED_LOAD_E_IMAGE;
    }
    REGUESTELF32DYNAMICStatus dynamic_status=re_guest_elf32_parse_dynamic(
        bytes,size,&image,&dynamic,&failed);
    if(dynamic_status!=RE_GUEST_ELF32_DYNAMIC_OK) {
        if(failed_index)*failed_index=failed;
        if(inner_status)*inner_status=(int)dynamic_status;
        return RE_GUEST_ELF32_LINKED_LOAD_E_DYNAMIC;
    }
    REGUESTELF32LINKStatus link_status=re_guest_elf32_parse_link_metadata(
        bytes,size,&image,&dynamic,&link,
        workspace->symbols,workspace->symbol_capacity,
        workspace->decoded_relocations,workspace->decoded_relocation_capacity,
        &failed);
    if(link_status!=RE_GUEST_ELF32_LINK_OK) {
        if(failed_index)*failed_index=failed;
        if(inner_status)*inner_status=(int)link_status;
        return RE_GUEST_ELF32_LINKED_LOAD_E_METADATA;
    }
    size_t symbol_count=link.has_symbols?link.symbol_count:0u;
    REGUESTELF32RELOCPLANStatus plan_status=re_guest_elf32_prepare_relocations(
        &dynamic,workspace->symbols,symbol_count,
        workspace->decoded_relocations,link.relocation_count,
        providers,provider_count,bias,
        workspace->prepared_relocations,workspace->prepared_relocation_capacity,
        &failed);
    if(plan_status!=RE_GUEST_ELF32_RELOC_PLAN_OK) {
        if(failed_index)*failed_index=failed;
        if(inner_status)*inner_status=(int)plan_status;
        return RE_GUEST_ELF32_LINKED_LOAD_E_RELOCATION_PLAN;
    }

    memset(&load_request,0,sizeof(load_request));
    load_request.elf_bytes=bytes;
    load_request.elf_size=size;
    load_request.load_bias=bias;
    load_request.buffers=buffers;
    load_request.buffer_count=buffer_count;
    load_request.relocations=workspace->prepared_relocations;
    load_request.relocation_count=link.relocation_count;
    REGUESTELF32LOADStatus load_status=re_guest_elf32_load_image(
        memory,&load_request,&pending.load,&failed,&failed_relocation);
    if(load_status!=RE_GUEST_ELF32_LOAD_OK) {
        if(failed_index)*failed_index=(load_status==RE_GUEST_ELF32_LOAD_E_RELOCATION)?failed_relocation:failed;
        if(inner_status)*inner_status=(int)load_status;
        return RE_GUEST_ELF32_LINKED_LOAD_E_LOAD;
    }
    pending.link=link;
    pending.dynamic_dependency_count=dynamic.needed_count;
    *result=pending;
    if(failed_index)*failed_index=link.relocation_count;
    return RE_GUEST_ELF32_LINKED_LOAD_OK;
}
