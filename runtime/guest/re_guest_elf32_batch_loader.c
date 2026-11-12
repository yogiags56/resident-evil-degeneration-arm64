#include "re_guest_elf32_batch_loader.h"
#include "re_guest_elf32_image.h"
#include <string.h>
#define ADDRESS_LIMIT 0x100000000ULL
typedef struct { uint32_t base; uint64_t size; } RANGE;
static int overlaps(uint64_t a,uint64_t as,uint64_t b,uint64_t bs) {
    return a < b+bs && b < a+as;
}

static REGUESTELF32BATCHStatus validate_memory_state(const REGUESTMEMORY *memory) {
    size_t active_count=0u;
    if(memory->count>RE_GUEST_MEMORY_MAX_REGIONS)
        return RE_GUEST_ELF32_BATCH_E_MEMORY_STATE;
    for(size_t i=0;i<RE_GUEST_MEMORY_MAX_REGIONS;i++) {
        const REGUESTMEMRegion *r=&memory->regions[i];
        uint64_t end;
        uintptr_t host_start;
        if(r->active>1u)
            return RE_GUEST_ELF32_BATCH_E_MEMORY_STATE;
        if(!r->active) continue;
        active_count++;
        if(!r->host||!r->size||r->size>(uint64_t)SIZE_MAX||
           (r->permissions&~(uint32_t)(RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE|RE_GUEST_MEM_EXEC))||
           !r->permissions||r->memory_type<RE_GUEST_MEM_TYPE_UNKNOWN||
           r->memory_type>RE_GUEST_MEM_TYPE_STRONGLY_ORDERED)
            return RE_GUEST_ELF32_BATCH_E_MEMORY_STATE;
        end=(uint64_t)r->base+r->size;
        if(end>ADDRESS_LIMIT)
            return RE_GUEST_ELF32_BATCH_E_MEMORY_STATE;
        host_start=(uintptr_t)r->host;
        if(r->size>(uint64_t)(UINTPTR_MAX-host_start))
            return RE_GUEST_ELF32_BATCH_E_MEMORY_STATE;
        for(size_t j=0;j<i;j++) {
            const REGUESTMEMRegion *p=&memory->regions[j];
            uint64_t p_end;
            if(!p->active) continue;
            p_end=(uint64_t)p->base+p->size;
            if((uint64_t)r->base<p_end&&(uint64_t)p->base<end)
                return RE_GUEST_ELF32_BATCH_E_MEMORY_STATE;
        }
    }
    if(active_count!=memory->count)
        return RE_GUEST_ELF32_BATCH_E_MEMORY_STATE;
    return RE_GUEST_ELF32_BATCH_OK;
}
static REGUESTELF32BATCHStatus preflight(REGUESTMEMORY *memory,
    const REGUESTELF32BATCHITEM *items,size_t count,size_t *segments,
    size_t *failed) {
    RANGE ranges[RE_GUEST_MEMORY_MAX_REGIONS];
    size_t range_count=0;
    for(size_t i=0;i<count;i++) {
        const REGUESTELF32LOADREQUEST *q=items[i].request;
        REGUESTELF32IMAGE image;
        size_t load_count=0,buffer_index=0;
        if(!q||!q->elf_bytes||(!q->buffers&&q->buffer_count)||
           (!q->relocations&&q->relocation_count)||
           q->buffer_count>RE_GUEST_ELF32_MAX_LOAD_SEGMENTS||
           q->relocation_count>RE_GUEST_ELF32_MAX_RELOCATIONS) {
            if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_ARGUMENT;
        }
        if(re_guest_elf32_parse_image(q->elf_bytes,q->elf_size,&image,NULL)!=RE_GUEST_ELF32_OK||
           image.load_overlap_count) {
            if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_IMAGE;
        }
        if(image.type==RE_GUEST_ELF32_ET_EXEC&&q->load_bias) {
            if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_LAYOUT;
        }
        for(size_t p=0;p<image.program_header_count;p++) {
            const REGUESTELF32PROGRAMHEADER *ph=&image.program_headers[p];
            uint64_t base,end;
            uint32_t flags;
            if(ph->type!=RE_GUEST_ELF32_PT_LOAD||!ph->memory_size)continue;
            if(buffer_index>=q->buffer_count||range_count>=RE_GUEST_MEMORY_MAX_REGIONS) {
                if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_CAPACITY;
            }
            if(ph->alignment>1u&&q->load_bias%ph->alignment) {
                if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_LAYOUT;
            }
            base=(uint64_t)ph->virtual_address+q->load_bias;
            end=base+ph->memory_size;
            if(base>UINT32_MAX||end>ADDRESS_LIMIT||
               !q->buffers[buffer_index].host_buffer||
               q->buffers[buffer_index].capacity<ph->memory_size) {
                if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_LAYOUT;
            }
            flags=ph->flags;
            if((flags&~(uint32_t)(RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_W|RE_GUEST_ELF32_PF_X))||
               !(flags&(RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_W|RE_GUEST_ELF32_PF_X))||
               ((flags&RE_GUEST_ELF32_PF_W)&&(flags&RE_GUEST_ELF32_PF_X))) {
                if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_LAYOUT;
            }
            for(size_t k=0;k<RE_GUEST_MEMORY_MAX_REGIONS;k++) {
                const REGUESTMEMRegion *r=&memory->regions[k];
                if(r->active&&overlaps(base,ph->memory_size,r->base,r->size)) {
                    if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_LAYOUT;
                }
            }
            for(size_t k=0;k<range_count;k++) if(overlaps(base,ph->memory_size,ranges[k].base,ranges[k].size)) {
                if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_LAYOUT;
            }
            ranges[range_count++]=(RANGE){(uint32_t)base,ph->memory_size};
            load_count++;buffer_index++;
        }
        if(!load_count||buffer_index!=q->buffer_count) {
            if(failed)*failed=i;return RE_GUEST_ELF32_BATCH_E_LAYOUT;
        }
    }
    if(range_count>RE_GUEST_MEMORY_MAX_REGIONS-memory->count)
        return RE_GUEST_ELF32_BATCH_E_CAPACITY;
    *segments=range_count;
    return RE_GUEST_ELF32_BATCH_OK;
}
REGUESTELF32BATCHStatus re_guest_elf32_load_batch(
    REGUESTMEMORY *memory,const REGUESTELF32BATCHITEM *items,size_t count,
    REGUESTELF32BATCHRESULT *result,size_t *failed_module,
    size_t *failed_segment,size_t *failed_relocation,int *inner_status) {
    REGUESTMEMORY snapshot;
    REGUESTELF32BATCHRESULT pending;
    size_t total=0;
    if(failed_module)*failed_module=0;
    if(failed_segment)*failed_segment=0;
    if(failed_relocation)*failed_relocation=0;
    if(inner_status)*inner_status=0;
    if(result)memset(result,0,sizeof(*result));
    memset(&pending,0,sizeof(pending));
    if(!memory||!items||!count||count>RE_GUEST_ELF32_MAX_BATCH_MODULES||!result)
        return RE_GUEST_ELF32_BATCH_E_ARGUMENT;
    REGUESTELF32BATCHStatus ps=validate_memory_state(memory);
    if(ps!=RE_GUEST_ELF32_BATCH_OK)return ps;
    ps=preflight(memory,items,count,&total,failed_module);
    if(ps!=RE_GUEST_ELF32_BATCH_OK)return ps;
    snapshot=*memory;
    for(size_t i=0;i<count;i++) {
        size_t seg=0,rel=0;
        REGUESTELF32LOADStatus ls=re_guest_elf32_load_image(memory,items[i].request,
            &pending.modules[i],&seg,&rel);
        if(ls!=RE_GUEST_ELF32_LOAD_OK) {
            *memory=snapshot;
            if(failed_module)*failed_module=i;
            if(failed_segment)*failed_segment=seg;
            if(failed_relocation)*failed_relocation=rel;
            if(inner_status)*inner_status=(int)ls;
            return RE_GUEST_ELF32_BATCH_E_LOAD;
        }
    }
    pending.module_count=count;
    pending.total_segments=total;
    *result=pending;
    if(failed_module)*failed_module=count;
    return RE_GUEST_ELF32_BATCH_OK;
}
