#include "re_guest_elf32_pipeline.h"
#include "re_guest_elf32_reloc_plan.h"
#include <string.h>

static REGUESTELF32PIPELINEStatus fail(
    REGUESTELF32PIPELINERESULT *out,size_t *failed_module,
    size_t *failed_index,int *inner_status,
    REGUESTELF32PIPELINEStatus status,size_t module,size_t index,int detail) {
    if(failed_module)*failed_module=module;
    if(failed_index)*failed_index=index;
    if(inner_status)*inner_status=detail;
    if(out)memset(out,0,sizeof(*out));
    return status;
}

REGUESTELF32PIPELINEStatus re_guest_elf32_load_pipeline(
    REGUESTMEMORY *memory,const REGUESTELF32PIPELINEMODULE *modules,
    size_t count,const char *root_name,REGUESTSYMBOL *provider_scratch,
    size_t provider_capacity,REGUESTELF32PIPELINERESULT *result,
    size_t *failed_module,size_t *failed_index,int *inner_status) {
    REGUESTELF32PIPELINERESULT pending;
    REGUESTELF32MODULEDESC desc[RE_GUEST_ELF32_MAX_MODULES];
    const char *needed_names[RE_GUEST_ELF32_MAX_MODULES][RE_GUEST_ELF32_MAX_MODULE_NEEDED];
    REGUESTELF32PROVIDERMODULE provider_modules[RE_GUEST_ELF32_MAX_MODULES];
    REGUESTELF32BATCHITEM batch[RE_GUEST_ELF32_MAX_MODULES];
    REGUESTELF32LOADREQUEST requests[RE_GUEST_ELF32_MAX_MODULES];
    REGUESTELF32BATCHRESULT loaded;
    const char *root_soname=NULL;
    size_t root_matches=0,root_module_index=0,provider_module_count=0,provider_count=0,failed=0;
    size_t provider_failed_symbol=0,batch_failed_segment=0,batch_failed_relocation=0;
    int batch_inner=0;

    if(failed_module)*failed_module=0;
    if(failed_index)*failed_index=0;
    if(inner_status)*inner_status=0;
    if(result)memset(result,0,sizeof(*result));
    memset(&pending,0,sizeof(pending));
    memset(desc,0,sizeof(desc));
    memset(needed_names,0,sizeof(needed_names));
    memset(provider_modules,0,sizeof(provider_modules));
    memset(batch,0,sizeof(batch));
    memset(requests,0,sizeof(requests));

    if(!memory||!modules||!count||count>RE_GUEST_ELF32_MAX_MODULES||
       !root_name||!root_name[0]||!result||
       (!provider_scratch&&provider_capacity))
        return RE_GUEST_ELF32_PIPELINE_E_ARGUMENT;

    /* Decode every candidate once. The caller owns all input and scratch memory. */
    for(size_t i=0;i<count;i++) {
        const REGUESTELF32PIPELINEMODULE *m=&modules[i];
        REGUESTELF32PIPELINEMETADATA *md=&pending.metadata[i];
        REGUESTELF32Status es;
        REGUESTELF32DYNAMICStatus ds;
        REGUESTELF32LINKStatus ls;
        const char *identity;

        if(!m->module_name||!m->module_name[0]||!m->elf_bytes||
           (!m->buffers&&m->buffer_count)||
           (!m->symbols&&m->symbol_capacity)||
           (!m->decoded_relocations&&m->decoded_relocation_capacity)||
           (!m->prepared_relocations&&m->prepared_relocation_capacity))
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_ARGUMENT,i,0,0);

        es=re_guest_elf32_parse_image(m->elf_bytes,m->elf_size,&md->image,&failed);
        if(es!=RE_GUEST_ELF32_OK)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_METADATA,i,failed,(int)es);

        ds=re_guest_elf32_parse_dynamic(m->elf_bytes,m->elf_size,
            &md->image,&md->dynamic,&failed);
        if(ds!=RE_GUEST_ELF32_DYNAMIC_OK)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_METADATA,i,failed,(int)ds);

        identity=md->dynamic.has_soname?md->dynamic.soname:m->module_name;
        if(!md->dynamic.has_soname&&strcmp(m->module_name,root_name)!=0)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_METADATA,i,0,
                RE_GUEST_ELF32_DYNAMIC_E_STRING);
        desc[i].soname=identity;
        desc[i].needed_count=md->dynamic.needed_count;
        for(size_t n=0;n<md->dynamic.needed_count;n++)
            needed_names[i][n]=md->dynamic.needed[n];
        desc[i].needed=needed_names[i];

        if(strcmp(m->module_name,root_name)==0||
           strcmp(identity,root_name)==0) {
            root_soname=identity;
            root_module_index=i;
            root_matches++;
        }

        ls=re_guest_elf32_parse_link_metadata(m->elf_bytes,m->elf_size,
            &md->image,&md->dynamic,&md->link,m->symbols,m->symbol_capacity,
            m->decoded_relocations,m->decoded_relocation_capacity,&failed);
        if(ls!=RE_GUEST_ELF32_LINK_OK)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_METADATA,i,failed,(int)ls);
    }

    if(root_matches!=1u)
        return fail(result,failed_module,failed_index,inner_status,
            RE_GUEST_ELF32_PIPELINE_E_ARGUMENT,0,0,
            (int)root_matches);

    REGUESTELF32MODULEGRAPHStatus gs=re_guest_elf32_build_module_graph(
        desc,count,root_soname,&pending.graph,&failed);
    if(gs!=RE_GUEST_ELF32_MODULE_GRAPH_OK)
        return fail(result,failed_module,failed_index,inner_status,
            RE_GUEST_ELF32_PIPELINE_E_GRAPH,failed,0,(int)gs);

    /* The inventory order is the current global provider lookup order.
       The graph order is independently dependency-first for loading. */
    for(size_t i=0;i<count;i++) {
        if(!pending.graph.reachable[i])continue;
        const REGUESTELF32PIPELINEMODULE *m=&modules[i];
        const REGUESTELF32PIPELINEMETADATA *md=&pending.metadata[i];
        provider_modules[provider_module_count++]=(REGUESTELF32PROVIDERMODULE){
            desc[i].soname,m->symbols,
            md->link.has_symbols?md->link.symbol_count:0u,m->load_bias};
    }

    REGUESTELF32PROVIDERStatus ps=re_guest_elf32_collect_providers(
        provider_modules,provider_module_count,provider_scratch,
        provider_capacity,&provider_count,&failed,&provider_failed_symbol);
    if(ps!=RE_GUEST_ELF32_PROVIDER_OK)
        return fail(result,failed_module,failed_index,inner_status,
            RE_GUEST_ELF32_PIPELINE_E_PROVIDER,failed,
            provider_failed_symbol,(int)ps);

    /* Prepare every module's relocations before the first guest mapping. */
    for(size_t i=0;i<count;i++) {
        if(!pending.graph.reachable[i])continue;
        const REGUESTELF32PIPELINEMODULE *m=&modules[i];
        REGUESTELF32PIPELINEMETADATA *md=&pending.metadata[i];
        size_t symbol_count=md->link.has_symbols?md->link.symbol_count:0u;
        REGUESTELF32RELOCPLANStatus rs=re_guest_elf32_prepare_relocations(
            &md->dynamic,m->symbols,symbol_count,m->decoded_relocations,
            md->link.relocation_count,provider_scratch,provider_count,
            m->load_bias,m->prepared_relocations,
            m->prepared_relocation_capacity,&failed);
        if(rs!=RE_GUEST_ELF32_RELOC_PLAN_OK)
            return fail(result,failed_module,failed_index,inner_status,
                RE_GUEST_ELF32_PIPELINE_E_RELOCATION,i,failed,(int)rs);
    }

    /* Assemble one dependency-first batch; requests remain alive through call. */
    for(size_t k=0;k<pending.graph.order_count;k++) {
        size_t i=pending.graph.order[k];
        const REGUESTELF32PIPELINEMODULE *m=&modules[i];
        const REGUESTELF32PIPELINEMETADATA *md=&pending.metadata[i];
        requests[k].elf_bytes=m->elf_bytes;
        requests[k].elf_size=m->elf_size;
        requests[k].load_bias=m->load_bias;
        requests[k].buffers=m->buffers;
        requests[k].buffer_count=m->buffer_count;
        requests[k].relocations=m->prepared_relocations;
        requests[k].relocation_count=md->link.relocation_count;
        batch[k].request=&requests[k];
    }

    REGUESTELF32BATCHStatus bs=re_guest_elf32_load_batch(
        memory,batch,pending.graph.order_count,&loaded,&failed,
        &batch_failed_segment,&batch_failed_relocation,&batch_inner);
    if(bs!=RE_GUEST_ELF32_BATCH_OK)
        return fail(result,failed_module,failed_index,inner_status,
            RE_GUEST_ELF32_PIPELINE_E_BATCH,failed,
            batch_failed_relocation,(int)(batch_inner?batch_inner:bs));

    pending.loaded=loaded;
    pending.module_count=count;
    pending.provider_count=provider_count;
    pending.root_module_index=root_module_index;
    *result=pending;
    if(failed_module)*failed_module=count;
    if(failed_index)*failed_index=0;
    if(inner_status)*inner_status=0;
    return RE_GUEST_ELF32_PIPELINE_OK;
}
