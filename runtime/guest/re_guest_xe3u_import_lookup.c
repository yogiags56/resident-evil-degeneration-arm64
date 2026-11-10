#include "re_guest_xe3u_import_lookup.h"
#include <stdint.h>
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static int verify_stub(const uint8_t *code,size_t code_size,
    uint32_t cell_offset,uint32_t *stub_out) {
    uint32_t delta,slot,stub,first,second,third,add2,effective;
    if(cell_offset<0x13b708u)return 0;
    delta=cell_offset-0x13b708u;
    if((delta&3u)!=0u)return 0;
    slot=delta/4u;
    if(slot>(UINT32_MAX-0x84u)/12u)return 0;
    stub=0x84u+slot*12u;
    if((uint64_t)stub+12u>code_size)return 0;
    first=rd32(code+stub);second=rd32(code+stub+4u);
    third=rd32(code+stub+8u);
    if(first!=0xe28fc601u)return 0;
    if(second==0xe28cca3bu)add2=0x3b000u;
    else if(second==0xe28cca3au)add2=0x3a000u;
    else return 0;
    if((third&0xfffff000u)!=0xe5bcf000u)return 0;
    effective=stub+8u+0x100000u+add2+(third&0xfffu);
    if(effective!=cell_offset)return 0;
    *stub_out=stub;
    return 1;
}
REGUESTXE3UIMPORTLOOKUPStatus re_guest_xe3u_lookup_import_cell(
    const uint8_t *code,size_t code_size,const REGUESTXE3UHEADER *header,
    const REGUESTXE3USYMBOL *symbols,size_t symbol_count,
    const REGUESTXE3UEXTERNALRELOCATION *external,size_t external_count,
    uint32_t runtime_base,uint32_t r12_value,
    REGUESTXE3UIMPORTCELL *result,size_t *failed_index) {
    uint64_t runtime_end;
    uint32_t offset,stub=0;
    size_t found=SIZE_MAX,found_index=0;
    if(failed_index)*failed_index=external_count;
    if(!code||!header||(!symbols&&symbol_count)||(!external&&external_count)||
       !result)return RE_GUEST_XE3U_IMPORT_LOOKUP_E_ARGUMENT;
    if(code_size!=header->code_file_size||runtime_base&0xfffu)
        return RE_GUEST_XE3U_IMPORT_LOOKUP_E_ARGUMENT;
    runtime_end=(uint64_t)runtime_base+header->code_memory_size;
    if(runtime_end>0x100000000ULL||r12_value<runtime_base||
       (uint64_t)r12_value+4u>runtime_end)
        return RE_GUEST_XE3U_IMPORT_LOOKUP_E_RANGE;
    offset=r12_value-runtime_base;
    if((offset&3u)!=0u||(uint64_t)offset+4u>code_size)
        return RE_GUEST_XE3U_IMPORT_LOOKUP_E_RANGE;
    for(size_t i=0;i<external_count;i++) {
        if(external[i].offset==offset) {
            if(found!=SIZE_MAX) {
                if(failed_index)*failed_index=i;
                return RE_GUEST_XE3U_IMPORT_LOOKUP_E_DUPLICATE;
            }
            found=i;found_index=i;
        }
    }
    if(found==SIZE_MAX)return RE_GUEST_XE3U_IMPORT_LOOKUP_E_NOT_FOUND;
    if(failed_index)*failed_index=found_index;
    if(external[found].section_type!=4u)
        return RE_GUEST_XE3U_IMPORT_LOOKUP_E_UNSUPPORTED;
    if(external[found].symbol_index>=symbol_count)
        return RE_GUEST_XE3U_IMPORT_LOOKUP_E_SYMBOL;
    if(rd32(code+offset)!=header->base_address+0x70u||
       !verify_stub(code,code_size,offset,&stub))
        return RE_GUEST_XE3U_IMPORT_LOOKUP_E_STUB;
    *result=(REGUESTXE3UIMPORTCELL){
        offset,stub,external[found].symbol_index,external[found].section_type};
    return RE_GUEST_XE3U_IMPORT_LOOKUP_OK;
}
