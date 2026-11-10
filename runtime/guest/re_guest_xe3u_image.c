#include "re_guest_xe3u_image.h"
#include <string.h>

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(p[0]|((uint16_t)p[1]<<8));
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static int range_ok(size_t size,uint32_t offset,uint32_t length) {
    return (uint64_t)offset+length<=size;
}
REGUESTXE3UStatus re_guest_xe3u_parse_header(
    const uint8_t *bytes,size_t size,REGUESTXE3UHEADER *header,
    size_t *failed_field) {
    REGUESTXE3UHEADER p;
    uint64_t end;
    if(failed_field)*failed_field=0;
    if(!bytes||!header)return RE_GUEST_XE3U_E_ARGUMENT;
    if(size<RE_GUEST_XE3U_HEADER_SIZE||memcmp(bytes,"XE3U",4)!=0)
        return RE_GUEST_XE3U_E_HEADER;
    memset(&p,0,sizeof(p));
    p.version=rd32(bytes+4);p.flags=rd16(bytes+8);
    p.architecture=rd16(bytes+10);
    p.fixup_offset=rd32(bytes+12);p.fixup_size=rd32(bytes+16);
    p.code_offset=rd32(bytes+20);p.code_file_size=rd32(bytes+24);
    p.code_memory_size=rd32(bytes+28);
    p.signature_offset=rd32(bytes+32);p.signature_size=rd32(bytes+36);
    p.entry_offset=rd32(bytes+40);p.config_offset=rd32(bytes+44);
    p.config_size=rd32(bytes+48);p.base_address=rd32(bytes+52);
    p.extra_offset=rd32(bytes+56);p.extra_size=rd32(bytes+60);
    p.extended_header_size=rd32(bytes+64);
    if(p.extended_header_size>=8u) {
        p.data_offset=rd32(bytes+68);p.has_data_offset=1u;
        if(p.data_offset>size) {
            if(failed_field)*failed_field=17u;
            return RE_GUEST_XE3U_E_RANGE;
        }
    }
    if(p.fixup_offset<RE_GUEST_XE3U_HEADER_SIZE||!p.fixup_size||
       !range_ok(size,p.fixup_offset,p.fixup_size)) {
        if(failed_field)*failed_field=4u;
        return RE_GUEST_XE3U_E_RANGE;
    }
    if(!p.code_file_size||p.code_file_size>p.code_memory_size||
       !range_ok(size,p.code_offset,p.code_file_size)) {
        if(failed_field)*failed_field=6u;
        return RE_GUEST_XE3U_E_RANGE;
    }
    if((uint64_t)p.base_address+p.code_memory_size>0x100000000ULL||
       p.entry_offset>=p.code_memory_size) {
        if(failed_field)*failed_field=9u;
        return RE_GUEST_XE3U_E_RANGE;
    }
    if(p.signature_size&&!range_ok(size,p.signature_offset,p.signature_size)) {
        if(failed_field)*failed_field=10u;
        return RE_GUEST_XE3U_E_RANGE;
    }
    if(p.config_size&&!range_ok(size,p.config_offset,p.config_size)) {
        if(failed_field)*failed_field=12u;
        return RE_GUEST_XE3U_E_RANGE;
    }
    if(p.extra_size&&!range_ok(size,p.extra_offset,p.extra_size)) {
        if(failed_field)*failed_field=16u;
        return RE_GUEST_XE3U_E_RANGE;
    }
    end=(uint64_t)p.code_offset+p.code_file_size;
    if(end>size)return RE_GUEST_XE3U_E_RANGE;
    *header=p;
    return RE_GUEST_XE3U_OK;
}

static REGUESTXE3UStatus scan_symbol_names(
    const uint8_t *bytes,size_t begin,size_t end,uint16_t expected,
    size_t *failed_record) {
    size_t cursor=begin;
    for(size_t i=0;i<expected;i++) {
        const uint8_t *nul;
        size_t n;
        if(cursor>=end)return RE_GUEST_XE3U_E_SYMBOL;
        nul=(const uint8_t *)memchr(bytes+cursor,0,end-cursor);
        if(!nul)return RE_GUEST_XE3U_E_SYMBOL;
        n=(size_t)(nul-(bytes+cursor));
        if(!n||n>=RE_GUEST_XE3U_SYMBOL_NAME_CAP) {
            if(failed_record)*failed_record=i;
            return RE_GUEST_XE3U_E_SYMBOL;
        }
        for(size_t j=0;j<n;j++)if(bytes[cursor+j]>0x7fu) {
            if(failed_record)*failed_record=i;
            return RE_GUEST_XE3U_E_SYMBOL;
        }
        cursor+=n+1u;
    }
    return RE_GUEST_XE3U_OK;
}
REGUESTXE3UStatus re_guest_xe3u_parse_fixups(
    const uint8_t *bytes,size_t size,const REGUESTXE3UHEADER *header,
    REGUESTXE3USYMBOL *symbols,size_t symbol_capacity,
    uint32_t *internal_offsets,size_t internal_capacity,
    REGUESTXE3UEXTERNALRELOCATION *external,size_t external_capacity,
    REGUESTXE3UFIXUPINFO *info,size_t *failed_section,size_t *failed_record) {
    REGUESTXE3UFIXUPINFO pending;
    size_t pos,end,symbol_count=0,internal_count=0,external_count=0;
    size_t symbol_payload=0,symbol_end=0;
    int has_internal_section=0;
    if(failed_section)*failed_section=0;
    if(failed_record)*failed_record=0;
    if(!bytes||!header||!info||
       (!symbols&&symbol_capacity)||(!internal_offsets&&internal_capacity)||
       (!external&&external_capacity))
        return RE_GUEST_XE3U_E_ARGUMENT;
    memset(&pending,0,sizeof(pending));
    if(!header->fixup_size||!range_ok(size,header->fixup_offset,header->fixup_size))
        return RE_GUEST_XE3U_E_RANGE;
    pos=header->fixup_offset;end=pos+header->fixup_size;
    /* Pass 1: validate subsection boundaries and determine bounded counts. */
    while(pos<end) {
        uint32_t type,section_size,count=0;
        size_t payload,section_end;
        if(end-pos<8u)return RE_GUEST_XE3U_E_FIXUP;
        type=rd32(bytes+pos);section_size=rd32(bytes+pos+4u);
        if(section_size<8u||(uint64_t)pos+section_size>end)
            return RE_GUEST_XE3U_E_FIXUP;
        payload=pos+8u;section_end=pos+section_size;
        if(pending.section_count>=RE_GUEST_XE3U_MAX_FIXUP_SECTIONS)
            return RE_GUEST_XE3U_E_CAPACITY;
        if(type==0u) {
            if(symbol_payload)return RE_GUEST_XE3U_E_DUPLICATE;
            if(section_end-payload<2u)return RE_GUEST_XE3U_E_FIXUP;
            count=rd16(bytes+payload);
            REGUESTXE3UStatus ss=scan_symbol_names(bytes,payload+2u,
                section_end,(uint16_t)count,failed_record);
            if(ss!=RE_GUEST_XE3U_OK) {
                if(failed_section)*failed_section=pending.section_count;
                return ss;
            }
            symbol_payload=payload+2u;symbol_end=section_end;
            symbol_count=count;
        } else if(type==1u) {
            if(has_internal_section)return RE_GUEST_XE3U_E_DUPLICATE;
            has_internal_section=1;
            if(section_end-payload<4u)return RE_GUEST_XE3U_E_FIXUP;
            count=rd32(bytes+payload);
            if((uint64_t)count*4u>section_end-(payload+4u))
                return RE_GUEST_XE3U_E_FIXUP;
            internal_count=count;
        } else if(type==2u||type==3u||type==4u) {
            if(section_end-payload<4u)return RE_GUEST_XE3U_E_FIXUP;
            count=rd32(bytes+payload);
            if((uint64_t)count*6u>section_end-(payload+4u))
                return RE_GUEST_XE3U_E_FIXUP;
            if(count>SIZE_MAX-external_count)return RE_GUEST_XE3U_E_CAPACITY;
            external_count+=count;
        }
        pending.sections[pending.section_count++]=(REGUESTXE3UFIXUPSECTION){
            type,(uint32_t)pos,section_size,count};
        pos=section_end;
    }
    if(pos!=end)return RE_GUEST_XE3U_E_FIXUP;
    if(symbol_count>symbol_capacity||internal_count>internal_capacity||
       external_count>external_capacity)
        return RE_GUEST_XE3U_E_CAPACITY;
    if(external_count&&!symbol_count)return RE_GUEST_XE3U_E_SYMBOL;
    /* Pass 2: validate every external symbol index before publishing output. */
    for(size_t s=0;s<pending.section_count;s++) {
        const REGUESTXE3UFIXUPSECTION *section=&pending.sections[s];
        if(section->type==2u||section->type==3u||section->type==4u) {
            size_t p=(size_t)section->offset+12u;
            for(size_t n=0;n<section->count;n++,p+=6u) {
                uint16_t symbol_index=rd16(bytes+p+4u);
                if(symbol_index>=symbol_count) {
                    if(failed_section)*failed_section=s;
                    if(failed_record)*failed_record=n;
                    return RE_GUEST_XE3U_E_SYMBOL;
                }
            }
        }
    }
    /* Pass 3: publish caller arrays after the complete input has passed. */
    if(symbol_count) {
        size_t cursor=symbol_payload;
        for(size_t n=0;n<symbol_count;n++) {
            const uint8_t *nul=(const uint8_t *)memchr(bytes+cursor,0,
                symbol_end-cursor);
            size_t length=(size_t)(nul-(bytes+cursor));
            memcpy(symbols[n].name,bytes+cursor,length);
            symbols[n].name[length]='\0';
            cursor+=length+1u;
        }
    }
    pos=0;
    for(size_t s=0;s<pending.section_count;s++) {
        const REGUESTXE3UFIXUPSECTION *section=&pending.sections[s];
        size_t payload=(size_t)section->offset+8u;
        if(section->type==1u) {
            for(size_t n=0;n<section->count;n++)
                internal_offsets[pos++]=rd32(bytes+payload+4u+n*4u);
        }
    }
    pos=0;
    for(size_t s=0;s<pending.section_count;s++) {
        const REGUESTXE3UFIXUPSECTION *section=&pending.sections[s];
        if(section->type==2u||section->type==3u||section->type==4u) {
            size_t p=(size_t)section->offset+12u;
            for(size_t n=0;n<section->count;n++,p+=6u) {
                uint16_t hi=rd16(bytes+p),lo=rd16(bytes+p+2u);
                external[pos++]=(REGUESTXE3UEXTERNALRELOCATION){
                    ((uint32_t)hi<<16)|lo,rd16(bytes+p+4u),(uint8_t)section->type};
            }
        }
    }
    pending.symbol_count=symbol_count;
    pending.internal_relocation_count=internal_count;
    pending.external_relocation_count=external_count;
    *info=pending;
    return RE_GUEST_XE3U_OK;
}
