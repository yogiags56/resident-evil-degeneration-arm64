#include "re_guest_elf32_reloc_plan.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    REGUESTELF32DYNAMIC dynamic={0};
    REGUESTELF32LINKSYMBOL symbols[4]={0};
    REGUESTELF32LINKRELOCATION input[4]={0};
    REGUESTSYMBOL providers[1]={{"foo",0x9000,RE_GUEST_STB_GLOBAL,
        RE_GUEST_STV_DEFAULT,1,NULL,NULL}};
    REGUESTRELOCATION output[4],before[4];
    size_t failed=99;
    dynamic.entry_count=1;dynamic.entries[0].tag=RE_GUEST_ELF32_DT_NULL;
    symbols[1].name[0]='x';symbols[1].name[1]=0;
    symbols[1].binding=RE_GUEST_STB_GLOBAL;symbols[1].section_index=1;
    symbols[1].value=0x1000;
    strcpy(symbols[2].name,"foo");symbols[2].binding=RE_GUEST_STB_GLOBAL;
    strcpy(symbols[3].name,"weak");symbols[3].binding=RE_GUEST_STB_WEAK;
    input[0]=(REGUESTELF32LINKRELOCATION){0x2000,0,RE_ARM_R_RELATIVE,0,0,0};
    input[1]=(REGUESTELF32LINKRELOCATION){0x2004,1,RE_ARM_R_ABS32,-4,1,0};
    input[2]=(REGUESTELF32LINKRELOCATION){0x2008,2,RE_ARM_R_GLOB_DAT,0,1,0};
    input[3]=(REGUESTELF32LINKRELOCATION){0x200c,3,RE_ARM_R_JUMP_SLOT,0,1,1};
    memset(output,0xa5,sizeof(output));
    assert(re_guest_elf32_prepare_relocations(&dynamic,symbols,4,input,4,
        providers,1,0x100000,output,4,&failed)==RE_GUEST_ELF32_RELOC_PLAN_OK);
    assert(failed==4);
    assert(output[0].target==0x102000&&output[0].type==RE_ARM_R_RELATIVE);
    assert(output[1].target==0x102004&&output[1].symbol_value==0x101000);
    assert(output[1].has_explicit_addend&&output[1].addend==(uint32_t)-4);
    assert(output[2].symbol_value==0x9000&&output[2].symbol_resolved);
    assert(output[3].symbol_value==0&&output[3].symbol_resolved);
    memcpy(before,output,sizeof(output));

    input[2].symbol_index=99;
    assert(re_guest_elf32_prepare_relocations(&dynamic,symbols,4,input,4,
        providers,1,0x100000,output,4,&failed)==RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL);
    assert(failed==2&&memcmp(output,before,sizeof(output))==0);

    input[2].symbol_index=2;dynamic.entries[0].tag=0x6ffffffeu;
    assert(re_guest_elf32_prepare_relocations(&dynamic,symbols,4,input,4,
        providers,1,0x100000,output,4,&failed)==RE_GUEST_ELF32_RELOC_PLAN_E_UNSUPPORTED);
    assert(memcmp(output,before,sizeof(output))==0);
    dynamic.entries[0].tag=RE_GUEST_ELF32_DT_NULL;

    input[0].offset=0xfffffff0u;
    assert(re_guest_elf32_prepare_relocations(&dynamic,symbols,4,input,1,
        providers,1,0x100000,output,4,&failed)==RE_GUEST_ELF32_RELOC_PLAN_E_RANGE);
    assert(memcmp(output,before,sizeof(output))==0);
    input[0].offset=0x2000;input[0].symbol_index=1;
    assert(re_guest_elf32_prepare_relocations(&dynamic,symbols,4,input,1,
        providers,1,0x100000,output,4,&failed)==RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL);

    /* Caller-built dynamic tables must retain a bounded DT_NULL terminator. */
    dynamic.entries[0].tag=RE_GUEST_ELF32_DT_NEEDED;
    assert(re_guest_elf32_prepare_relocations(&dynamic,symbols,4,input,1,
        providers,1,0x100000,output,4,&failed)==RE_GUEST_ELF32_RELOC_PLAN_E_ARGUMENT);
    assert(memcmp(output,before,sizeof(output))==0);
    dynamic.entries[0].tag=RE_GUEST_ELF32_DT_NULL;

    /* Non-data/non-callable ELF symbol kinds cannot drive relocations. */
    symbols[2].type=3u; /* STT_SECTION */
    input[0].symbol_index=2;
    assert(re_guest_elf32_prepare_relocations(&dynamic,symbols,4,input,1,
        providers,1,0x100000,output,4,&failed)==RE_GUEST_ELF32_RELOC_PLAN_E_UNSUPPORTED);
    assert(failed==0&&memcmp(output,before,sizeof(output))==0);
    symbols[2].type=0u;

    /* Caller-provided unterminated names must fail before any output write. */
    memset(symbols[2].name, 'X', sizeof(symbols[2].name));
    input[0]=(REGUESTELF32LINKRELOCATION){0x2000,2,RE_ARM_R_GLOB_DAT,0,1,0};
    assert(re_guest_elf32_prepare_relocations(&dynamic,symbols,4,input,1,
        providers,1,0x100000,output,4,&failed)==RE_GUEST_ELF32_RELOC_PLAN_E_SYMBOL);
    assert(failed==0&&memcmp(output,before,sizeof(output))==0);
    puts("Guest ELF32 relocation planning tests: PASS");return 0;
}
