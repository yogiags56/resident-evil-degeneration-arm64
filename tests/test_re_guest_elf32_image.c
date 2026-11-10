#include "re_guest_elf32_image.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t v, int little) {
    if (little) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
    else { p[0]=(uint8_t)(v>>8); p[1]=(uint8_t)v; }
}
static void put32(uint8_t *p, uint32_t v, int little) {
    if (little) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
    else { p[0]=(uint8_t)(v>>24); p[1]=(uint8_t)(v>>16); p[2]=(uint8_t)(v>>8); p[3]=(uint8_t)v; }
}
static void phdr(uint8_t *p, uint32_t type, uint32_t offset,
    uint32_t vaddr, uint32_t filesz, uint32_t memsz, uint32_t flags,
    uint32_t align, int little) {
    put32(p,type,little); put32(p+4,offset,little); put32(p+8,vaddr,little);
    put32(p+12,vaddr,little); put32(p+16,filesz,little);
    put32(p+20,memsz,little); put32(p+24,flags,little); put32(p+28,align,little);
}
static void valid_elf(uint8_t *b, int little) {
    memset(b,0,160); b[0]=0x7f; b[1]='E'; b[2]='L'; b[3]='F';
    b[4]=1; b[5]=(uint8_t)(little?1:2); b[6]=1;
    put16(b+16,RE_GUEST_ELF32_ET_EXEC,little);
    put16(b+18,RE_GUEST_ELF32_EM_ARM,little);
    put32(b+20,1,little); put32(b+24,0x1000,little);
    put32(b+28,52,little); put16(b+40,52,little);
    put16(b+42,32,little); put16(b+44,3,little);
    phdr(b+52,RE_GUEST_ELF32_PT_LOAD,0,0x1000,4,8,
        RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_X,0x1000,little);
    phdr(b+84,RE_GUEST_ELF32_PT_LOAD,0x80,0x2000,4,12,
        RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_W,4,little);
    phdr(b+116,RE_GUEST_ELF32_PT_DYNAMIC,0x90,0x3000,8,8,
        RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_W,4,little);
}
int main(void) {
    uint8_t bytes[160]; REGUESTELF32IMAGE image; size_t failed=999;
    valid_elf(bytes,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_OK);
    assert(image.type==RE_GUEST_ELF32_ET_EXEC && image.machine==RE_GUEST_ELF32_EM_ARM);
    assert(image.little_endian==1 && image.load_count==2 && image.dynamic_count==1);
    assert(image.entry_in_executable_load==1 && failed==3);

    valid_elf(bytes,0);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_OK);
    assert(image.little_endian==0 && image.entry==0x1000 && failed==3);

    valid_elf(bytes,1); bytes[4]=2;
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_E_CLASS);
    valid_elf(bytes,1); put16(bytes+18,183,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_E_MACHINE);
    valid_elf(bytes,1); put16(bytes+44,33,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_E_CAPACITY);
    valid_elf(bytes,1); put32(bytes+84+16,13,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_E_SEGMENT_SIZE && failed==1);
    valid_elf(bytes,1); put32(bytes+84+4,0xfffffff0u,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_E_FILE_RANGE && failed==1);
    valid_elf(bytes,1); put32(bytes+84+8,0xfffffff8u,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_E_MEMORY_RANGE && failed==1);
    valid_elf(bytes,1); put32(bytes+84+28,3,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_E_ALIGNMENT && failed==1);
    valid_elf(bytes,1); put32(bytes+84+8,0x1004,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_OK);
    assert(image.load_overlap_count==1u);
    valid_elf(bytes,1); put32(bytes+24,0x5000,1);
    assert(re_guest_elf32_parse_image(bytes,sizeof(bytes),&image,&failed)==RE_GUEST_ELF32_OK);
    assert(image.entry_in_executable_load==0);
    puts("Guest ELF32 program-header parser tests: PASS");
    return 0;
}
