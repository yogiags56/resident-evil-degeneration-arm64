#include "re_guest_elf32_pipeline.h"
#include <string.h>
static REGUESTELF32PIPELINEStatus fail(REGUESTELF32PIPELINERESULT *out,
    size_t *fm,size_t *fi,int *inner,REGUESTELF32PIPELINEStatus status,
    size_t module,size_t index,int detail) {
    if(fm)*fm=module;if(fi)*fi=index;if(inner)*inner=detail;
    if(out)memset(out,0,sizeof(*out));
    return status;
}
REGUESTELF32PIPELINEStatus re_guest_elf32_load_pipeline(
    REGUESTMEMORY *memory,const REGUESTELF32PIPELINEMODULE *modules,
    size_t count,const char *root,REGUESTSYMBOL *provider_scratch,
    size_t provider_capacity,REGUESTELF32PIPELINERESULT *result,
    size_t *failed_module,size_t *failed_index,int *inner_status) {
    REGUESTELF32PIPELINERESULT pending;
    REGUESTELF32MODULEDESC desc[RE_GUEST_ELF32_MAX_MODULES];
    REGUESTELF32PROVIDERMODULE provider_modules[RE_GUEST_ELF32_MAX_MODULES];
    REGUESTELF32BATCHITEM batch[RE_GUEST_ELF32_MAX_MODULES];
    REGUESTELF32BATCHRESULT loaded;
    size_t provider_module_count=0,provider_count=0,failed=0;
    if(failed_module)*failed_module=0;if(failed_index)*failed_index=0;
    if(inner_status)*inner_status=0;if(result)memset(result,0,sizeof(*result));
    memset(&pending,0,sizeof(pending));
    if(!memory||!modules||!count||count>RE_GUEST_ELF32_MAX_MODULES||
       !root||!root[0]||!result||(!provider_scratch&&provider_capacity))
        return RE_GUEST_ELF32_PIPELINE_E_ARGUMENT;
    memset(desc,0,sizeof(desc));memset(provider_modules,0,sizeof(provider_modules));
    memset(batch,0,sizeof(batch));
    /* Parse every candidate once; construct inventory names and dependency edges. */
    for(size_t i=0;i<count;i++) {
        REGUESTELF32PIPELINEMODULE const *m=&modules[i];
        REGUESTELF32PIPELINEMETADATA *md=&pending.metadata[i];
        if(!m->module_name||!m->module_name[0]||!m->elf_bytes||
           (!m->buffers&&m->buffer_count)||(!m->symbols&&m->symbol_capacity)||
           (!m->decoded_relocations&&m->decoded_relocation_capacity)||
           (!m->prepared_relocations&&m->prepared_relocation_capacity))
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_ARGUMENT,i,0,0);
        REGUESTELF32Status es=re_guest_elf32_parse_image(m->elf_bytes,m->elf_size,&md->image,&failed);
        if(es!=RE_GUEST_ELF32_OK)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_METADATA,i,failed,(int)es);
        REGUESTELF32DYNAMICStatus ds=re_guest_elf32_parse_dynamic(m->elf_bytes,m->elf_size,
            &md->image,&md->dynamic,&failed);
        if(ds!=RE_GUEST_ELF32_DYNAMIC_OK)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_METADATA,i,failed,(int)ds);
        const char *name=md->dynamic.has_soname?md->dynamic.soname:m->module_name;
        if(!md->dynamic.has_soname&&strcmp(m->module_name,root)!=0)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_METADATA,i,0,RE_GUEST_ELF32_DYNAMIC_E_STRING);
        desc[i].soname=name;
        desc[i].needed_count=md->dynamic.needed_count;
        desc[i].needed=(const char *const *)md->dynamic.needed;
        REGUESTELF32LINKStatus ls=re_guest_elf32_parse_link_metadata(
            m->elf_bytes,m->elf_size,&md->image,&md->dynamic,&md->link,
            m->symbols,m->symbol_capacity,m->decoded_relocations,
            m->decoded_relocation_capacity,&failed);
        if(ls!=RE_GUEST_ELF32_LINK_OK)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_METADATA,i,failed,(int)ls);
    }
    REGUESTELF32MODULEGRAPHStatus gs=re_guest_elf32_build_module_graph(
        desc,count,root,&pending.graph,&failed);
    if(gs!=RE_GUEST_ELF32_MODULE_GRAPH_OK)
        return fail(result,failed_module,failed_index,inner_status,
            RE_GUEST_ELF32_PIPELINE_E_GRAPH,failed,0,(int)gs);
    /* Provider lookup order is the caller's module inventory order; unreachable
       candidates are excluded, while load order remains graph dependency-first. */
    for(size_t i=0;i<count;i++)if(pending.graph.reachable[i]) {
        REGUESTELF32PIPELINEMODULE const *m=&modules[i];
        REGUESTELF32PIPELINEMETADATA const *md=&pending.metadata[i];
        provider_modules[provider_module_count++]=(REGUESTELF32PROVIDERMODULE){
            desc[i].soname,m->symbols,md->link.has_symbols?md->link.symbol_count:0u,m->load_bias};
    }
    REGUESTELF32PROVIDERStatus ps=re_guest_elf32_collect_providers(
        provider_modules,provider_module_count,provider_scratch,provider_capacity,
        &provider_count,&failed,&failed_index[0]);
    if(ps!=RE_GUEST_ELF32_PROVIDER_OK)
        return fail(result,failed_module,failed_index,inner_status,
            RE_GUEST_ELF32_PIPELINE_E_PROVIDER,failed,failed_index?*failed_index:0,(int)ps);
    /* Plan every module's relocations before any guest memory mutation. */
    for(size_t i=0;i<count;i++)if(pending.graph.reachable[i]) {
        REGUESTELF32PIPELINEMODULE const *m=&modules[i];
        REGUESTELF32PIPELINEMETADATA *md=&pending.metadata[i];
        size_t symbol_count=md->link.has_symbols?md->link.symbol_count:0u;
        REGUESTELF32RELOCPLANStatus rs=re_guest_elf32_prepare_relocations(
            &md->dynamic,m->symbols,symbol_count,m->decoded_relocations,
            md->link.relocation_count,provider_scratch,provider_count,m->load_bias,
            m->prepared_relocations,m->prepared_relocation_capacity,&failed);
        if(rs!=RE_GUEST_ELF32_RELOC_PLAN_OK)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_RELOCATION,i,failed,(int)rs);
    }
    for(size_t k=0;k<pending.graph.order_count;k++) {
        size_t i=pending.graph.order[k];
        REGUESTELF32PIPELINEMODULE const *m=&modules[i];
        REGUESTELF32PIPELINEMETADATA const *md=&pending.metadata[i];
        REGUESTELF32LOADREQUEST *q=(REGUESTELF32LOADREQUEST *)0;
        (void)q;
        /* Batch request records are assembled in dependency-first graph order. */
        static REGUESTELF32LOADREQUEST requests[RE_GUEST_ELF32_MAX_MODULES];
        requests[k].elf_bytes=m->elf_bytes;requests[k].elf_size=m->elf_size;
        requests[k].load_bias=m->load_bias;requests[k].buffers=m->buffers;
        requests[k].buffer_count=m->buffer_count;
        requests[k].relocations=m->prepared_relocations;
        requests[k].relocation_count=md->link.relocation_count;
        batch[k].request=&requests[k];
    }
    REGUESTELF32BATCHStatus bs=re_guest_elf32_load_batch(memory,batch,
        pending.graph.order_count,&loaded,&failed,&failed_index[0],inner_status);
    if(bs!=RE_GUEST_ELF32_BATCH_OK)
        return fail(result,failed_module,failed_index,inner_status,
            RE_GUEST_ELF32_PIPELINE_E_BATCH,failed,failed_index?*failed_index:0,(int)bs);
    pending.loaded=loaded;pending.module_count=count;pending.provider_count=provider_count;
    *result=pending;
    if(failed_module)*failed_module=count;if(failed_index)*failed_index=0;
    return RE_GUEST_ELF32_PIPELINE_OK;
}
