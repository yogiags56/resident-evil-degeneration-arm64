#include "re_guest_elf32_provider_catalog.h"
#include <string.h>
#define SHN_UNDEF 0u
#define SHN_ABS 0xfff1u
#define STT_TLS 6u
#define MAX_PROVIDERS 65536u
static int valid_soname(const char *s) {
    return s&&s[0]&&!strchr(s,'/')&&!strchr(s,'\\')&&
        strlen(s)<RE_GUEST_ELF32_DYNAMIC_STRING_CAP;
}
REGUESTELF32PROVIDERStatus re_guest_elf32_collect_providers(
    const REGUESTELF32PROVIDERMODULE *modules,size_t module_count,
    REGUESTSYMBOL *providers,size_t capacity,size_t *provider_count,
    size_t *failed_module,size_t *failed_symbol) {
    size_t needed=0;
    if(provider_count)*provider_count=0;
    if(failed_module)*failed_module=0;
    if(failed_symbol)*failed_symbol=0;
    if(!modules||!module_count||module_count>RE_GUEST_ELF32_MAX_PROVIDER_MODULES||
       (!providers&&capacity)||!provider_count)
        return RE_GUEST_ELF32_PROVIDER_E_ARGUMENT;
    for(size_t i=0;i<module_count;i++) {
        const REGUESTELF32PROVIDERMODULE *m=&modules[i];
        if(!valid_soname(m->soname)||(!m->symbols&&m->symbol_count)||
           m->symbol_count>RE_GUEST_ELF32_LINK_MAX_SYMBOLS) {
            if(failed_module)*failed_module=i;
            return RE_GUEST_ELF32_PROVIDER_E_NAME;
        }
        for(size_t j=0;j<m->symbol_count;j++) {
            const REGUESTELF32LINKSYMBOL *s=&m->symbols[j];
            uint64_t value;
            if(s->binding>RE_GUEST_STB_WEAK||s->visibility>RE_GUEST_STV_PROTECTED) {
                if(failed_module)*failed_module=i;
                if(failed_symbol)*failed_symbol=j;
                return RE_GUEST_ELF32_PROVIDER_E_SYMBOL;
            }
            if(s->section_index==SHN_UNDEF||s->binding==RE_GUEST_STB_LOCAL||
               (s->visibility!=RE_GUEST_STV_DEFAULT&&s->visibility!=RE_GUEST_STV_PROTECTED)||
               s->type==STT_TLS||!s->name[0])continue;
            if(s->binding!=RE_GUEST_STB_GLOBAL&&s->binding!=RE_GUEST_STB_WEAK)continue;
            value=s->value;
            if(s->section_index!=SHN_ABS)value+=m->load_bias;
            if(value>UINT32_MAX) {
                if(failed_module)*failed_module=i;
                if(failed_symbol)*failed_symbol=j;
                return RE_GUEST_ELF32_PROVIDER_E_RANGE;
            }
            if(++needed>MAX_PROVIDERS)return RE_GUEST_ELF32_PROVIDER_E_CAPACITY;
        }
    }
    if(needed>capacity)return RE_GUEST_ELF32_PROVIDER_E_CAPACITY;
    size_t out=0;
    for(size_t i=0;i<module_count;i++) {
        const REGUESTELF32PROVIDERMODULE *m=&modules[i];
        for(size_t j=0;j<m->symbol_count;j++) {
            const REGUESTELF32LINKSYMBOL *s=&m->symbols[j];
            uint64_t value;
            if(s->section_index==SHN_UNDEF||s->binding==RE_GUEST_STB_LOCAL||
               (s->visibility!=RE_GUEST_STV_DEFAULT&&s->visibility!=RE_GUEST_STV_PROTECTED)||
               s->type==STT_TLS||!s->name[0]||
               (s->binding!=RE_GUEST_STB_GLOBAL&&s->binding!=RE_GUEST_STB_WEAK))continue;
            value=s->value;
            if(s->section_index!=SHN_ABS)value+=m->load_bias;
            REGUESTSYMBOL *p=&providers[out++];
            p->name=s->name;p->value=(uint32_t)value;p->binding=s->binding;
            p->visibility=s->visibility;p->defined=1u;
            p->version_name=NULL;p->version_library=m->soname;
        }
    }
    *provider_count=out;
    if(failed_module)*failed_module=module_count;
    if(failed_symbol)*failed_symbol=0;
    return RE_GUEST_ELF32_PROVIDER_OK;
}
