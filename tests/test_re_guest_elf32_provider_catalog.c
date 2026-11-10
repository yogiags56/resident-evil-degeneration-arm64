#include "re_guest_elf32_provider_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define ABS 0xfff1u
int main(void) {
    REGUESTELF32LINKSYMBOL a[8]={0},b[2]={0};
    REGUESTELF32PROVIDERMODULE modules[2]={
        {"libfirst.so",a,8,0x100000},
        {"libsecond.so",b,2,0x200000}
    };
    REGUESTSYMBOL out[8],before[8];
    size_t count=99,fm=99,fs=99;
    strcpy(a[0].name,"strong");a[0].binding=RE_GUEST_STB_GLOBAL;
    a[0].visibility=RE_GUEST_STV_DEFAULT;a[0].section_index=1;a[0].value=0x100;
    strcpy(a[1].name,"weak");a[1].binding=RE_GUEST_STB_WEAK;
    a[1].visibility=RE_GUEST_STV_DEFAULT;a[1].section_index=1;a[1].value=0x200;
    strcpy(a[2].name,"absolute");a[2].binding=RE_GUEST_STB_GLOBAL;
    a[2].visibility=RE_GUEST_STV_DEFAULT;a[2].section_index=ABS;a[2].value=0x300;
    strcpy(a[3].name,"local");a[3].binding=RE_GUEST_STB_LOCAL;
    a[3].visibility=RE_GUEST_STV_DEFAULT;a[3].section_index=1;a[3].value=0x400;
    strcpy(a[4].name,"hidden");a[4].binding=RE_GUEST_STB_GLOBAL;
    a[4].visibility=RE_GUEST_STV_HIDDEN;a[4].section_index=1;a[4].value=0x500;
    strcpy(a[5].name,"tls");a[5].binding=RE_GUEST_STB_GLOBAL;
    a[5].visibility=RE_GUEST_STV_DEFAULT;a[5].section_index=1;a[5].type=6;a[5].value=0x600;
    strcpy(a[6].name,"undefined");a[6].binding=RE_GUEST_STB_GLOBAL;
    a[6].visibility=RE_GUEST_STV_DEFAULT;a[6].section_index=0;
    strcpy(a[7].name,"weak");a[7].binding=RE_GUEST_STB_GLOBAL;
    a[7].visibility=RE_GUEST_STV_DEFAULT;a[7].section_index=1;a[7].value=0x700;
    strcpy(b[0].name,"second");b[0].binding=RE_GUEST_STB_GLOBAL;
    b[0].visibility=RE_GUEST_STV_DEFAULT;b[0].section_index=1;b[0].value=0x80;
    strcpy(b[1].name,"weak");b[1].binding=RE_GUEST_STB_WEAK;
    b[1].visibility=RE_GUEST_STV_DEFAULT;b[1].section_index=1;b[1].value=0x90;
    memset(out,0xa5,sizeof(out));
    assert(re_guest_elf32_collect_providers(modules,2,out,8,&count,&fm,&fs)==RE_GUEST_ELF32_PROVIDER_OK);
    assert(count==6&&fm==2);
    assert(!strcmp(out[0].name,"strong")&&out[0].value==0x100100);
    assert(!strcmp(out[1].name,"weak")&&out[1].value==0x100200);
    assert(!strcmp(out[2].name,"absolute")&&out[2].value==0x300);
    assert(!strcmp(out[3].name,"weak")&&out[3].value==0x100700);
    assert(!strcmp(out[4].name,"second")&&out[4].value==0x200080);
    assert(!strcmp(out[5].name,"weak")&&out[5].value==0x200090);
    assert(!strcmp(out[0].version_library,"libfirst.so"));
    memcpy(before,out,sizeof(out));
    assert(re_guest_elf32_collect_providers(modules,2,out,4,&count,&fm,&fs)==RE_GUEST_ELF32_PROVIDER_E_CAPACITY);
    assert(count==0&&memcmp(out,before,sizeof(out))==0);
    a[0].value=0xfffffff0u;
    assert(re_guest_elf32_collect_providers(modules,2,out,8,&count,&fm,&fs)==RE_GUEST_ELF32_PROVIDER_E_RANGE);
    assert(fm==0&&fs==0&&count==0&&memcmp(out,before,sizeof(out))==0);
    puts("Guest ELF32 provider catalog tests: PASS");
    return 0;
}
