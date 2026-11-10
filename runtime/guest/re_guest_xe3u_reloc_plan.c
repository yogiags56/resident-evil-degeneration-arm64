#include "re_guest_xe3u_reloc_plan.h"
#include <stdint.h>
#include <string.h>
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static size_t hash_slot(uint32_t offset,size_t capacity) {
    uint32_t key=offset>>2;
    return (size_t)(key*2654435761u)%capacity;
}
REGUESTXE3URELOCPLANStatus re_guest_xe3u_prepare_internal_relocations(
    const uint8_t *code,size_t code_size,const REGUESTXE3UHEADER *header,
    const uint32_t *offsets,size_t count,uint32_t runtime_base,
    uint32_t *scratch,size_t scratch_capacity,
    REGUESTXE3URELOCATION *output,size_t output_capacity,
    size_t *failed_index) {
    uint64_t original_end,runtime_end;
    int64_t delta;
    if(failed_index)*failed_index=count;
    if(!code||!header||(!offsets&&count)||(!output&&count)||
       (!scratch&&count))return RE_GUEST_XE3U_RELOC_PLAN_E_ARGUMENT;
    if(header->code_file_size!=code_size||
       header->code_file_size>header->code_memory_size||
       !header->code_memory_size)
        return RE_GUEST_XE3U_RELOC_PLAN_E_ARGUMENT;
    if(count>SIZE_MAX/2u||scratch_capacity<count*2u||
       output_capacity<count)
        return RE_GUEST_XE3U_RELOC_PLAN_E_CAPACITY;
    if(count&&scratch_capacity==0u)
        return RE_GUEST_XE3U_RELOC_PLAN_E_CAPACITY;
    if((runtime_base&0xfffu)!=0u)
        return RE_GUEST_XE3U_RELOC_PLAN_E_ALIGNMENT;
    original_end=(uint64_t)header->base_address+header->code_memory_size;
    runtime_end=(uint64_t)runtime_base+header->code_memory_size;
    if(original_end>0x100000000ULL||runtime_end>0x100000000ULL)
        return RE_GUEST_XE3U_RELOC_PLAN_E_ADDRESS_OVERFLOW;
    delta=(int64_t)(uint64_t)runtime_base-(int64_t)(uint64_t)header->base_address;
    for(size_t i=0;i<scratch_capacity;i++)scratch[i]=UINT32_MAX;
    /* Full preflight: bounds, uniqueness, original targets, and new targets. */
    for(size_t i=0;i<count;i++) {
        uint32_t off=offsets[i],word,target,relocated,relocated_target;
        int64_t candidate;
        size_t slot;
        if((off&3u)!=0u||((uint64_t)off+4u)>code_size) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_RELOC_PLAN_E_PATCH_RANGE;
        }
        slot=hash_slot(off,scratch_capacity);
        while(scratch[slot]!=UINT32_MAX) {
            if(scratch[slot]==off) {
                if(failed_index)*failed_index=i;
                return RE_GUEST_XE3U_RELOC_PLAN_E_DUPLICATE;
            }
            slot=(slot+1u)%scratch_capacity;
        }
        scratch[slot]=off;
        word=rd32(code+off);
        target=word&~1u;
        if((uint64_t)target<header->base_address||
           (uint64_t)target>=original_end) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_RELOC_PLAN_E_TARGET_RANGE;
        }
        candidate=(int64_t)(uint64_t)word+delta;
        if(candidate<0||candidate>UINT32_MAX) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_RELOC_PLAN_E_ADDRESS_OVERFLOW;
        }
        relocated=(uint32_t)candidate;
        if((relocated&1u)!=(word&1u)) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_RELOC_PLAN_E_ALIGNMENT;
        }
        relocated_target=relocated&~1u;
        if((uint64_t)relocated_target<runtime_base||
           (uint64_t)relocated_target>=runtime_end) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_XE3U_RELOC_PLAN_E_TARGET_RANGE;
        }
    }
    /* No failure is possible after preflight; now publish all plan records. */
    for(size_t i=0;i<count;i++) {
        uint32_t off=offsets[i],word=rd32(code+off);
        output[i]=(REGUESTXE3URELOCATION){
            off,word,(uint32_t)((int64_t)(uint64_t)word+delta),
            (uint8_t)(word&1u)};
    }
    if(failed_index)*failed_index=count;
    return RE_GUEST_XE3U_RELOC_PLAN_OK;
}
