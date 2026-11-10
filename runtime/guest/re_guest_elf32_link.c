#include "re_guest_elf32_link.h"

#include <string.h>

enum {
    DT_PLTRELSZ=2, DT_HASH=4, DT_STRTAB=5, DT_SYMTAB=6,
    DT_RELA=7, DT_RELASZ=8, DT_RELAENT=9, DT_STRSZ=10,
    DT_SYMENT=11, DT_REL=17, DT_RELSZ=18, DT_RELENT=19,
    DT_PLTREL=20, DT_JMPREL=23
};

static uint16_t rd16(const uint8_t *p, int le) {
    return le ? (uint16_t)(p[0] | ((uint16_t)p[1] << 8))
              : (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}
static uint32_t rd32(const uint8_t *p, int le) {
    if (le) return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
        ((uint32_t)p[2] << 8) | p[3];
}
static int tag_value(const REGUESTELF32DYNAMIC *d, uint32_t tag,
                     uint32_t *value, int *present) {
    *present = 0; *value = 0;
    for (size_t i=0; i<d->entry_count; ++i) {
        if (d->entries[i].tag != tag) continue;
        if (*present) return 0;
        *present = 1; *value = d->entries[i].value;
    }
    return 1;
}
static int file_range(const REGUESTELF32IMAGE *im, size_t size,
                      uint32_t address, uint32_t length, uint32_t *offset) {
    uint64_t end=(uint64_t)address+length, found=0;
    size_t matches=0;
    if (end > 0x100000000ULL) return 0;
    for(size_t i=0;i<im->program_header_count;i++) {
        const REGUESTELF32PROGRAMHEADER *p=&im->program_headers[i];
        uint64_t start=p->virtual_address, pend=start+p->file_size;
        if(p->type==RE_GUEST_ELF32_PT_LOAD && address>=start && end<=pend) {
            uint64_t o=(uint64_t)p->offset+(address-p->virtual_address);
            if(o+length>size) return 0;
            found=o; matches++;
        }
    }
    if(matches!=1 || found>UINT32_MAX) return 0;
    *offset=(uint32_t)found; return 1;
}
static int copy_name(const uint8_t *bytes,size_t size,uint32_t str_off,
                     uint32_t str_size,uint32_t name_off,char *out) {
    uint64_t start=(uint64_t)str_off+name_off;
    const uint8_t *end; size_t n;
    if(name_off>=str_size || start>=size || (uint64_t)str_off+str_size>size)
        return 0;
    end=(const uint8_t *)memchr(bytes+(size_t)start,0,str_size-name_off);
    if(!end) return 0;
    n=(size_t)(end-(bytes+(size_t)start));
    if(n>=RE_GUEST_ELF32_LINK_SYMBOL_NAME_CAP) return 0;
    memcpy(out,bytes+(size_t)start,n);out[n]='\0';return 1;
}
static REGUESTELF32LINKStatus parse_reloc_table(
    const uint8_t *bytes,size_t size,const REGUESTELF32IMAGE *im,
    uint32_t address,uint32_t bytes_count,uint32_t entsize,int rela,int plt,
    REGUESTELF32LINKRELOCATION *out,size_t cap,size_t *used,size_t *failed) {
    uint32_t off; uint32_t expected=rela?12u:8u;
    if(!bytes_count) return RE_GUEST_ELF32_LINK_OK;
    if(entsize!=expected || bytes_count%entsize)
        return RE_GUEST_ELF32_LINK_E_FORMAT;
    if(*used>cap || bytes_count/entsize>cap-*used)
        return RE_GUEST_ELF32_LINK_E_CAPACITY;
    if(!file_range(im,size,address,bytes_count,&off))
        return RE_GUEST_ELF32_LINK_E_RANGE;
    for(uint32_t i=0;i<bytes_count/entsize;i++) {
        const uint8_t *p=bytes+off+(size_t)i*entsize;
        uint32_t ro=rd32(p,im->little_endian),ri=rd32(p+4,im->little_endian);
        REGUESTELF32LINKRELOCATION *r=&out[*used];
        r->offset=ro;r->symbol_index=ri>>8;r->type=ri&255u;
        r->addend=rela?(int32_t)rd32(p+8,im->little_endian):0;
        r->has_addend=(uint8_t)rela;r->is_plt=(uint8_t)plt;
        if(failed)*failed=*used;
        (*used)++;
    }
    return RE_GUEST_ELF32_LINK_OK;
}

REGUESTELF32LINKStatus re_guest_elf32_parse_link_metadata(
    const uint8_t *bytes,size_t size,const REGUESTELF32IMAGE *image,
    const REGUESTELF32DYNAMIC *dynamic,REGUESTELF32LINKINFO *info,
    REGUESTELF32LINKSYMBOL *symbols,size_t symbol_capacity,
    REGUESTELF32LINKRELOCATION *relocations,size_t relocation_capacity,
    size_t *failed_index) {
    uint32_t hash=0,strtab=0,strsz=0,symtab=0,syment=0;
    uint32_t rel=0,relsz=0,relent=0,rela=0,relasz=0,relaent=0;
    uint32_t jmprel=0,pltsz=0,pltrel=0,hashoff=0,stroff=0,symoff=0;
    int hp,sp,ssp,ep,rp,rsp,rep,rap,rasp,raep,jp,pp,tp,pltp;
    uint32_t nbucket,nchain;size_t used=0;
    REGUESTELF32LINKStatus st;
    if(failed_index)*failed_index=0;
    if(!bytes||!image||!dynamic||!info||(!symbols&&symbol_capacity)||
       (!relocations&&relocation_capacity)) return RE_GUEST_ELF32_LINK_E_ARGUMENT;
    memset(info,0,sizeof(*info));
    if(!dynamic->present) return RE_GUEST_ELF32_LINK_OK;
#define GET(tag,var,flag) do { if(!tag_value(dynamic,(tag),&(var),&(flag))) return RE_GUEST_ELF32_LINK_E_TAG; } while(0)
    GET(DT_HASH,hash,hp);GET(DT_STRTAB,strtab,sp);GET(DT_STRSZ,strsz,ssp);
    GET(DT_SYMTAB,symtab,ep);GET(DT_SYMENT,syment,rp);
    GET(DT_REL,rel,rsp);GET(DT_RELSZ,relsz,rep);GET(DT_RELENT,relent,rap);
    GET(DT_RELA,rela,rasp);GET(DT_RELASZ,relasz,raep);GET(DT_RELAENT,relaent,jp);
    GET(DT_JMPREL,jmprel,pp);GET(DT_PLTRELSZ,pltsz,tp);GET(DT_PLTREL,pltrel,pltp);
#undef GET
    if((sp||ssp||ep||rp) && !(sp&&ssp&&ep&&rp))
        return RE_GUEST_ELF32_LINK_E_TAG;
    if((rsp||rep||rap) && !(rsp&&rep&&rap))
        return RE_GUEST_ELF32_LINK_E_TAG;
    if((rasp||raep||jp) && !(rasp&&raep&&jp))
        return RE_GUEST_ELF32_LINK_E_TAG;
    if((pp||tp||pltp) && !(pp&&tp&&pltp))
        return RE_GUEST_ELF32_LINK_E_TAG;
    if((pp||tp||pltp) && !hp) return RE_GUEST_ELF32_LINK_E_UNSUPPORTED;
    if((sp&&syment!=16u)||(rsp&&relent!=8u)||(rasp&&relaent!=12u))
        return RE_GUEST_ELF32_LINK_E_FORMAT;
    if(!hp && (sp||rsp||rasp||pp))
        return RE_GUEST_ELF32_LINK_E_UNSUPPORTED;
    if(hp) {
        if(!file_range(image,size,hash,8u,&hashoff))
            return RE_GUEST_ELF32_LINK_E_RANGE;
        nbucket=rd32(bytes+hashoff,image->little_endian);
        nchain=rd32(bytes+hashoff+4u,image->little_endian);
        if(!nbucket||!nchain||nchain>RE_GUEST_ELF32_LINK_MAX_SYMBOLS)
            return nchain>RE_GUEST_ELF32_LINK_MAX_SYMBOLS?
                RE_GUEST_ELF32_LINK_E_CAPACITY:RE_GUEST_ELF32_LINK_E_FORMAT;
        uint64_t hash_bytes=8ULL+4ULL*((uint64_t)nbucket+nchain);
        if(hash_bytes>UINT32_MAX || !file_range(image,size,hash,(uint32_t)hash_bytes,&hashoff))
            return RE_GUEST_ELF32_LINK_E_RANGE;
        info->sysv_hash_address=hash;info->has_sysv_hash=1;
        info->symbol_count=nchain;
    }
    if(sp) {
        if(!dynamic->has_string_table || dynamic->string_table_virtual_address!=strtab ||
           dynamic->string_table_size!=strsz || !strsz ||
           !file_range(image,size,strtab,strsz,&stroff))
            return RE_GUEST_ELF32_LINK_E_RANGE;
        if(!file_range(image,size,symtab,nchain*16u,&symoff))
            return RE_GUEST_ELF32_LINK_E_RANGE;
        if(symbol_capacity<nchain) return RE_GUEST_ELF32_LINK_E_CAPACITY;
        for(uint32_t i=0;i<nchain;i++) {
            const uint8_t *p=bytes+symoff+(size_t)i*16u;
            REGUESTELF32LINKSYMBOL *s=&symbols[i];
            s->name_offset=rd32(p,image->little_endian);
            s->value=rd32(p+4,image->little_endian);
            s->size=rd32(p+8,image->little_endian);
            s->binding=p[12]>>4;s->type=p[12]&15u;s->visibility=p[13]&3u;
            s->section_index=rd16(p+14,image->little_endian);
            if(s->name_offset && !copy_name(bytes,size,stroff,strsz,s->name_offset,s->name))
                return RE_GUEST_ELF32_LINK_E_STRING;
            if(s->binding>2u || s->visibility>3u)
                return RE_GUEST_ELF32_LINK_E_SYMBOL;
        }
        info->has_symbols=1;info->symbol_table_address=symtab;
        info->symbol_entry_size=syment;info->string_table_address=strtab;
        info->string_table_size=strsz;
    }
    if((rsp&&relsz)||(rasp&&relasz)||(pp&&pltsz)) {
        if(!relocations) return RE_GUEST_ELF32_LINK_E_ARGUMENT;
        if(rsp&&relsz) {
            st=parse_reloc_table(bytes,size,image,rel,relsz,relent,0,0,
                relocations,relocation_capacity,&used,failed_index);
            if(st) return st;
            info->has_rel=1;
        }
        if(rasp&&relasz) {
            st=parse_reloc_table(bytes,size,image,rela,relasz,relaent,1,0,
                relocations,relocation_capacity,&used,failed_index);
            if(st) return st;
            info->has_rela=1;
        }
        if(pp&&pltsz) {
            if(pltrel!=DT_REL && pltrel!=DT_RELA)
                return RE_GUEST_ELF32_LINK_E_FORMAT;
            st=parse_reloc_table(bytes,size,image,jmprel,pltsz,
                pltrel==DT_REL?relent:relaent,pltrel==DT_RELA,1,
                relocations,relocation_capacity,&used,failed_index);
            if(st) return st;
            info->has_plt=1;info->plt_relocation_count=pltsz/
                (pltrel==DT_REL?relent:relaent);
        }
    }
    for(size_t i=0;i<used;i++)
        if(info->has_symbols && relocations[i].symbol_index>=info->symbol_count)
            {if(failed_index)*failed_index=i;return RE_GUEST_ELF32_LINK_E_RELOCATION;}
    info->relocation_count=(uint32_t)used;
    return RE_GUEST_ELF32_LINK_OK;
}
