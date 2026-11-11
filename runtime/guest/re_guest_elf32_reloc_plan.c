#include "re_guest_elf32_reloc_plan.h"

#include <string.h>

#define RE_GUEST_ELF32_RELOC_MAX 4096u
#define RE_ELF32_SHN_ABS 0xfff1u
#define RE_ELF32_STT_TLS 6u
#define RE_ELF32_STT_NOTYPE 0u
#define RE_ELF32_STT_OBJECT 1u
#define RE_ELF32_STT_FUNC 2u
#define RE_ELF32_DT_VERSYM 0x6ffffff0u
#define RE_ELF32_DT_VERDEF 0x6ffffffcu
#define RE_ELF32_DT_VERDEFNUM 0x6ffffffdu
#define RE_ELF32_DT_VERNEED 0x6ffffffeu
#define RE_ELF32_DT_VERNEEDNUM 0x6fffffffu

static int has_unsupported_version_tags(const REGUESTELF32DYNAMIC *d) {
    for (size_t i=0;i<d->entry_count;i++) {
        uint32_t t=d->entries[i].tag;
        if (t==RE_ELF32_DT_VERSYM || t==RE_ELF32_DT_VERDEF ||
            t==RE_ELF32_DT_VERDEFNUM || t==RE_ELF32_DT_VERNEED ||
            t==RE_ELF32_DT_VERNEEDNUM) return 1;
    }
    return 0;
}

REGUESTELF32RELOCPLANStatus re_guest_elf32_prepare_relocations(
    const REGUESTELF32DYNAMIC *dynamic,
    const REGUESTELF32LINKSYMBOL *symbols,size_t symbol_count,
    const REGUESTELF32LINKRELOCATION *input,size_t input_count,
    const REGUESTSYMBOL *providers,size_t provider_count,
    uint32_t bias,REGUESTRELOCATION *output,size_t output_capacity,
    size_t *failed_index) {
    REGUESTRELOCATION pending[RE_GUEST_ELF32_RELOC_MAX];
    if(failed_index)*failed_index=input_count;
    if(!dynamic || (!symbols&&symbol_count) || (!input&&input_count) ||
       (!providers&&provider_count) || (!output&&output_capacity) ||
       input_count>RE_GUEST_ELF32_RELOC_MAX)
        return RE_GUEST_ELF32_RELOC_PLAN_E_ARGUMENT;
    if(output_capacity<input_count)
        return RE_GUEST_ELF32_RELOC_PLAN_E_CAPACITY;
    if(dynamic->entry_count>RE_GUEST_ELF32_MAX_DYNAMIC_ENTRIES)
        return RE_GUEST_ELF32_RELOC_PLAN_E_ARGUMENT;
    /* Caller-built dynamic metadata must have the same bounded terminator
     * invariant as records produced by the ELF32 dynamic parser. */
    if ((dynamic->present && dynamic->entry_count==0u) ||
        (dynamic->entry_count &&
         dynamic->entries[dynamic->entry_count-1u].tag!=RE_GUEST_ELF32_DT_NULL))
        return RE_GUEST_ELF32_RELOC_PLAN_E_ARGUMENT;
    if(has_unsupported_version_tags(dynamic))
        return RE_GUEST_ELF32_RELOC_PLAN_E_UNSUPPORTED;

    for(size_t i=0;i<input_count;i++) {
        const REGUESTELF32LINKRELOCATION *in=&input[i];
        REGUESTRELOCATION *out=&pending[i];
        uint64_t target=(uint64_t)in->offset+bias;
        memset(out,0,sizeof(*out));
        if(target>UINT32_MAX) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_ELF32_RELOC_PLAN_E_RANGE;
        }
        out->target=(uint32_t)target;
        out->type=in->type;
        out->load_bias=bias;
        out->has_explicit_addend=in->has_addend;
        out->addend=(uint32_t)in->addend;
        if(in->has_addend>1u) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_ELF32_RELOC_PLAN_E_ARGUMENT;
        }
        if(in->type==RE_ARM_R_RELATIVE) {
            if(in->symbol_index!=0u) {
                if(failed_index)*failed_index=i;
                return RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL;
            }
            continue;
        }
        if(in->type!=RE_ARM_R_ABS32 && in->type!=RE_ARM_R_GLOB_DAT &&
           in->type!=RE_ARM_R_JUMP_SLOT) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_ELF32_RELOC_PLAN_E_UNSUPPORTED;
        }
        if(in->symbol_index>=symbol_count) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL;
        }
        const REGUESTELF32LINKSYMBOL *s=&symbols[in->symbol_index];
        /* This public planner also accepts caller-built symbol tables. */
        if (!memchr(s->name, '\0', sizeof(s->name)) ||
            s->binding > RE_GUEST_STB_WEAK ||
            s->visibility > RE_GUEST_STV_PROTECTED) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL;
        }
        if(s->type!=RE_ELF32_STT_NOTYPE &&
           s->type!=RE_ELF32_STT_OBJECT &&
           s->type!=RE_ELF32_STT_FUNC) {
            if(failed_index)*failed_index=i;
            return RE_GUEST_ELF32_RELOC_PLAN_E_UNSUPPORTED;
        }
        out->symbol_index=in->symbol_index;
        out->symbol_resolved=1u;
        if(s->section_index!=0u) {
            uint64_t value=s->value;
            if(s->section_index!=RE_ELF32_SHN_ABS)value+=bias;
            if(value>UINT32_MAX) {
                if(failed_index)*failed_index=i;
                return RE_GUEST_ELF32_RELOC_PLAN_E_RANGE;
            }
            out->symbol_value=(uint32_t)value;
        } else {
            REGUESTSYMBOLREQUEST request;
            REGUESTSYMBOLRESULT result;
            REGUESTSYMBOLStatus status;
            if(s->binding==RE_GUEST_STB_LOCAL || !s->name[0]) {
                if(failed_index)*failed_index=i;
                return RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL;
            }
            request.name=s->name;
            request.version_name=NULL;
            request.version_library=NULL;
            request.allow_unversioned_fallback=0u;
            status=re_guest_symbol_resolve(&request,providers,provider_count,&result);
            if(status==RE_GUEST_SYMBOL_E_NOT_FOUND &&
               s->binding==RE_GUEST_STB_WEAK) {
                out->symbol_value=0u;
            } else if(status!=RE_GUEST_SYMBOL_OK) {
                if(failed_index)*failed_index=i;
                return status==RE_GUEST_SYMBOL_E_NOT_FOUND ?
                    RE_GUEST_ELF32_RELOC_PLAN_E_UNRESOLVED :
                    RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL;
            } else {
                out->symbol_value=result.value;
            }
        }
    }
    if(input_count)memcpy(output,pending,input_count*sizeof(*output));
    return RE_GUEST_ELF32_RELOC_PLAN_OK;
}
