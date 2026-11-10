#include "re_guest_elf32_provider_catalog.h"
#include "re_guest_elf32_reloc_plan.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    REGUESTELF32LINKSYMBOL provider_symbols[1]={0},consumer_symbols[2]={0};
    REGUESTELF32PROVIDERMODULE modules[1];
    REGUESTSYMBOL providers[2];
    REGUESTELF32DYNAMIC dynamic={0};
    REGUESTELF32LINKRELOCATION decoded[1]={{0x2000,1,RE_ARM_R_GLOB_DAT,0,0,0}};
    REGUESTRELOCATION prepared[1];
    size_t count=0,failed=0;
    strcpy(provider_symbols[0].name,"s3eFileOpen");
    provider_symbols[0].binding=RE_GUEST_STB_GLOBAL;
    provider_symbols[0].visibility=RE_GUEST_STV_DEFAULT;
    provider_symbols[0].section_index=1;
    provider_symbols[0].value=0x3450;
    modules[0]=(REGUESTELF32PROVIDERMODULE){"libimagine.so",provider_symbols,1,0x400000};
    assert(re_guest_elf32_collect_providers(modules,1,providers,2,&count,NULL,NULL)==RE_GUEST_ELF32_PROVIDER_OK);
    assert(count==1&&providers[0].value==0x403450);
    strcpy(consumer_symbols[1].name,"s3eFileOpen");
    consumer_symbols[1].binding=RE_GUEST_STB_GLOBAL;
    consumer_symbols[1].section_index=0;
    dynamic.entry_count=1;dynamic.entries[0].tag=RE_GUEST_ELF32_DT_NULL;
    assert(re_guest_elf32_prepare_relocations(&dynamic,consumer_symbols,2,
        decoded,1,providers,count,0x800000,prepared,1,&failed)==RE_GUEST_ELF32_RELOC_PLAN_OK);
    assert(failed==1&&prepared[0].target==0x802000);
    assert(prepared[0].symbol_resolved&&prepared[0].symbol_value==0x403450);
    puts("Guest ELF32 cross-module relocation planning tests: PASS");
    return 0;
}
