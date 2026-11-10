#include "re_guest_xe3u_import_plan.h"
#include <stdint.h>
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static size_t hash_slot(uint32_t offset,size_t capacity) {
    uint32_t key=offset>>2;
    return (size_t)(key*2654435761u)%capacity;
}
static int verify_stub(const uint8_t *code,size_t code_size,uint32_t cell) {
    uint32_t delta,slot,stub,a,b,c,add2,effective;
    if(cell<0x13b708u)return 0;
    delta=cell-0x13b708u;
    if(delta&3u)return 0;
    slot=delta/4u;
    if(slot>(UINT32_MAX-0x84u)/12u)return 0;
    stub=0x84u+slot*12u;
    if((uint64_t)stub+12u>code_size)return 0;
    a=rd32(code+stub);b=rd32(code+stub+4u);c=rd32(code+stub+8u);
    if(a!=0xe28fc601u)return 0;
    if(b==0xe28cca3bu)add2=0x3b000u;
    else if(b==0xe28cca3au)add2=0x3a000u;
    else return 0;
    if((c&0xfffff000u)!=0xe5bcf000u)return 0;
    effective=stub+8u+0x100000u+add2+(c&0xfffu);
    return effective==cell;
}
static const REGUESTIMPORTBinding *find_binding(
    const REGUESTIMPORTTable *table,uint16_t symbol_index) {
    for(size_t i=0;i<table->count;i++)
        if(table->entries[i].symbol_index==symbol_index)return &table->entries[i];
    return NULL;
}
REGUESTXE3UIMPORTPLANStatus re_guest_xe3u_prepare_external_import_plan(
    const uint8_t *code,size_t code_size,const REGUESTXE3UHEADER *header,
    size_t symbol_count,const REGUESTXE3UEXTERNALRELOCATION *external,
    size_t external_count,const REGUESTIMPORTTable *imports,uint32_t runtime_base,
    uint32_t unresolved_pc,int allow_unbound,uint32_t *scratch,size_t scratch_cap,
    REGUESTXE3UEXTERNALPATCH *output,size_t output_cap,
    size_t *unbound_count,size_t *failed_index) {
    uint64_t image_end,original_end;
    size_t unbound=0;
    if(unbound_count)*unbound_count=0;
    if(failed_index)*failed_index=external_count;
    if(!code||!header||!external||!imports||(!scratch&&external_count)||
       (!output&&external_count))return RE_GUEST_XE3U_IMPORT_PLAN_E_ARGUMENT;
    if(code_size!=header->code_file_size||
       header->code_file_size>header->code_memory_size||
       (runtime_base&0xfffu)!=0u)
        return RE_GUEST_XE3U_IMPORT_PLAN_E_ARGUMENT;
    if(external_count>SIZE_MAX/2u||scratch_cap<external_count*2u||
       output_cap<external_count)
        return RE_GUEST_XE3U_IMPORT_PLAN_E_CAPACITY;
    image_end=(uint64_t)runtime_base+header->code_memory_size;
    original_end=(uint64_t)header->base_address+header->code_memory_size;
    if(image_end>0x100000000ULL||original_end>0x100000000ULL||
       header->code_memory_size<0x74u||imports->count>RE_IMPORT_MAX_BINDINGS)
        return RE_GUEST_XE3U_IMPORT_PLAN_E_RANGE;
    if(allow_unbound && (unresolved_pc==0u || (unresolved_pc&3u)!=0u ||
       ((uint64_t)unresolved_pc>=runtime_base&&(uint64_t)unresolved_pc<image_end)))
        return RE_GUEST_XE3U_IMPORT_PLAN_E_ARGUMENT;
    for(size_t i=0;i<scratch_cap;i++)scratch[i]=UINT32_MAX;
    /* Complete read-only preflight. */
    for(size_t i=0;i<external_count;i++) {
        const REGUESTXE3UEXTERNALRELOCATION *e=&external[i];
        const REGUESTIMPORTBinding *binding;
        uint32_t old,slot;
        if(e->section_type!=4u) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_IMPORT_PLAN_E_FORMAT;
        }
        if(e->symbol_index>=symbol_count||(e->offset&3u)!=0u||
           (uint64_t)e->offset+4u>code_size) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_IMPORT_PLAN_E_RANGE;
        }
        slot=hash_slot(e->offset,scratch_cap);
        while(scratch[slot]!=UINT32_MAX) {
            if(scratch[slot]==e->offset) {
                if(failed_index)*failed_index=i;
                return RE_GUEST_XE3U_IMPORT_PLAN_E_DUPLICATE;
            }
            slot=(slot+1u)%scratch_cap;
        }
        scratch[slot]=e->offset;
        old=rd32(code+e->offset);
        if(old==header->base_address+0x70u) {
            if(!verify_stub(code,code_size,e->offset)) {
                if(failed_index)*failed_index=i;
                return RE_GUEST_XE3U_IMPORT_PLAN_E_FORMAT;
            }
        } else if(old!=0u) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_IMPORT_PLAN_E_FORMAT;
        }
        binding=find_binding(imports,e->symbol_index);
        if(binding) {
            if((binding->trampoline_address&3u)!=0u ||
               (!binding->handler&&!binding->mapped_handler) ||
               ((uint64_t)binding->trampoline_address>=runtime_base&&
                (uint64_t)binding->trampoline_address<image_end)) {
                if(failed_index)*failed_index=i;
                return RE_GUEST_XE3U_IMPORT_PLAN_E_ARGUMENT;
            }
        } else if(!allow_unbound) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_IMPORT_PLAN_E_UNBOUND;
        } else {
            unbound++;
        }
    }
    /* Publish only after every record has passed preflight. */
    for(size_t i=0;i<external_count;i++) {
        const REGUESTXE3UEXTERNALRELOCATION *e=&external[i];
        const REGUESTIMPORTBinding *binding=find_binding(imports,e->symbol_index);
        uint32_t old=rd32(code+e->offset);
        uint32_t target=binding?binding->trampoline_address:unresolved_pc;
        output[i]=(REGUESTXE3UEXTERNALPATCH){
            e->offset,old,target|1u,e->symbol_index,e->section_type,
            (uint8_t)(old==header->base_address+0x70u)};
    }
    if(unbound_count)*unbound_count=unbound;
    if(failed_index)*failed_index=external_count;
    return RE_GUEST_XE3U_IMPORT_PLAN_OK;
}
REGUESTXE3UIMPORTPLANStatus re_guest_xe3u_apply_external_import_plan(
    REGUESTMEMBUS *bus,const REGUESTXE3UHEADER *header,uint32_t runtime_base,
    const REGUESTXE3UEXTERNALPATCH *plan,size_t count,size_t *failed_index) {
    uint64_t image_end;
    if(failed_index)*failed_index=count;
    if(!bus||!bus->memory||!header||(!plan&&count))
        return RE_GUEST_XE3U_IMPORT_PLAN_E_ARGUMENT;
    image_end=(uint64_t)runtime_base+header->code_memory_size;
    if(image_end>0x100000000ULL||(runtime_base&0xfffu)!=0u)
        return RE_GUEST_XE3U_IMPORT_PLAN_E_RANGE;
    /* Preflight every cell and every expected original value before writes. */
    for(size_t i=0;i<count;i++) {
        uint32_t address,value;
        if((plan[i].cell_offset&3u)!=0u||
           (uint64_t)plan[i].cell_offset+4u>header->code_file_size||
           (uint64_t)runtime_base+plan[i].cell_offset+4u>image_end||
           !(plan[i].trampoline_value&1u)) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_IMPORT_PLAN_E_RANGE;
        }
        address=runtime_base+plan[i].cell_offset;
        if((uint64_t)(plan[i].trampoline_value&~1u)>=runtime_base&&
           (uint64_t)(plan[i].trampoline_value&~1u)<image_end) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_IMPORT_PLAN_E_RANGE;
        }
        REGUESTMEMStatus ms=re_guest_memory_read32(bus,address,&value);
        if(ms!=RE_GUEST_MEM_OK) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_IMPORT_PLAN_E_MEMORY;
        }
        if(value!=plan[i].original_value) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_IMPORT_PLAN_E_STALE;
        }
    }
    for(size_t i=0;i<count;i++) {
        uint32_t address=runtime_base+plan[i].cell_offset;
        if(re_guest_memory_write32(bus,address,plan[i].trampoline_value)!=RE_GUEST_MEM_OK) {
            if(failed_index)*failed_index=i;
            for(size_t j=0;j<i;j++) {
                uint32_t rollback_address=runtime_base+plan[j].cell_offset;
                if(re_guest_memory_write32(bus,rollback_address,
                    plan[j].original_value)!=RE_GUEST_MEM_OK)
                    return RE_GUEST_XE3U_IMPORT_PLAN_E_ROLLBACK;
            }
            return RE_GUEST_XE3U_IMPORT_PLAN_E_MEMORY;
        }
    }
    return RE_GUEST_XE3U_IMPORT_PLAN_OK;
}
