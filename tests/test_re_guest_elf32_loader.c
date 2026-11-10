#include "re_guest_elf32_loader.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
}
static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static void make_elf(uint8_t elf[0x120], uint32_t first_flags) {
    memset(elf, 0, 0x120);
    elf[0]=0x7f; elf[1]='E'; elf[2]='L'; elf[3]='F';
    elf[4]=1; elf[5]=1; elf[6]=1;
    put16(elf+16, RE_GUEST_ELF32_ET_DYN);
    put16(elf+18, RE_GUEST_ELF32_EM_ARM);
    put32(elf+20, 1u);
    put32(elf+24, 0x1000u);
    put32(elf+28, 52u);
    put16(elf+40, 52u);
    put16(elf+42, 32u);
    put16(elf+44, 2u);

    /* Executable PT_LOAD. */
    put32(elf+52, RE_GUEST_ELF32_PT_LOAD);
    put32(elf+56, 0x100u);
    put32(elf+60, 0x1000u);
    put32(elf+68, 4u);
    put32(elf+72, 8u);
    put32(elf+76, first_flags);
    put32(elf+80, 4u);

    /* Writable PT_LOAD. */
    put32(elf+84, RE_GUEST_ELF32_PT_LOAD);
    put32(elf+88, 0x110u);
    put32(elf+92, 0x2000u);
    put32(elf+100, 4u);
    put32(elf+104, 8u);
    put32(elf+108, RE_GUEST_ELF32_PF_R | RE_GUEST_ELF32_PF_W);
    put32(elf+112, 4u);

    elf[0x100]=0x11; elf[0x101]=0x22; elf[0x102]=0x33; elf[0x103]=0x44;
    put32(elf+0x110, 0x10u);
}

static int test_load_and_relocate(void) {
    uint8_t elf[0x120], code[8]={0}, data[8]={0};
    REGUESTELF32LOADBUFFER buffers[2]={{code,sizeof(code)},{data,sizeof(data)}};
    REGUESTRELOCATION reloc={0};
    REGUESTELF32LOADREQUEST request={0};
    REGUESTELF32LOADRESULT result;
    REGUESTMEMORY memory;
    void *resolved=NULL;

    make_elf(elf, RE_GUEST_ELF32_PF_R | RE_GUEST_ELF32_PF_X);
    reloc.target=0x102000u;
    reloc.type=RE_ARM_R_RELATIVE;
    reloc.has_explicit_addend=0u;
    request.elf_bytes=elf; request.elf_size=sizeof(elf);
    request.load_bias=0x100000u;
    request.buffers=buffers; request.buffer_count=2u;
    request.relocations=&reloc; request.relocation_count=1u;
    re_guest_memory_init(&memory);

    assert(re_guest_elf32_load_image(&memory,&request,&result,NULL,NULL)==
        RE_GUEST_ELF32_LOAD_OK);
    assert(result.entry_valid && result.entry_point==0x101000u);
    assert(result.load_count==2u && !result.dynamic.present);
    assert(memcmp(code,elf+0x100,4u)==0 && code[4]==0u && code[7]==0u);
    assert(data[0]==0x10u && data[1]==0u && data[2]==0x10u && data[3]==0u && data[4]==0u);
    assert(re_guest_memory_resolve(&memory,0x101000u,4u,
        RE_GUEST_MEM_EXEC,&resolved)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_resolve(&memory,0x102000u,4u,
        RE_GUEST_MEM_WRITE,&resolved)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_resolve(&memory,0x101000u,4u,
        RE_GUEST_MEM_WRITE,&resolved)==RE_GUEST_MEM_E_PERMISSION);
    assert(re_guest_memory_region_count(&memory)==2u);
    assert(re_guest_memory_unmap(&memory,0x101000u)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_unmap(&memory,0x102000u)==RE_GUEST_MEM_OK);
    return 0;
}

static int test_relocation_failure_rolls_back_maps(void) {
    uint8_t elf[0x120], code[8]={0}, data[8]={0};
    REGUESTELF32LOADBUFFER buffers[2]={{code,sizeof(code)},{data,sizeof(data)}};
    REGUESTRELOCATION reloc={0};
    REGUESTELF32LOADREQUEST request={0};
    REGUESTELF32LOADRESULT result;
    REGUESTMEMORY memory;
    size_t failed=0u;

    make_elf(elf, RE_GUEST_ELF32_PF_R | RE_GUEST_ELF32_PF_X);
    reloc.target=0x102000u;
    reloc.type=0x7fffffffu;
    request.elf_bytes=elf; request.elf_size=sizeof(elf);
    request.load_bias=0x100000u;
    request.buffers=buffers; request.buffer_count=2u;
    request.relocations=&reloc; request.relocation_count=1u;
    re_guest_memory_init(&memory);

    assert(re_guest_elf32_load_image(&memory,&request,&result,NULL,&failed)==
        RE_GUEST_ELF32_LOAD_E_RELOCATION);
    assert(failed==0u);
    assert(re_guest_memory_region_count(&memory)==0u);
    return 0;
}

static int test_wx_and_short_buffer_rejected_before_mapping(void) {
    uint8_t elf[0x120], code[8]={0}, data[8]={0};
    REGUESTELF32LOADBUFFER buffers[2]={{code,4u},{data,sizeof(data)}};
    REGUESTELF32LOADREQUEST request={0};
    REGUESTELF32LOADRESULT result;
    REGUESTMEMORY memory;

    make_elf(elf, RE_GUEST_ELF32_PF_R | RE_GUEST_ELF32_PF_W |
        RE_GUEST_ELF32_PF_X);
    request.elf_bytes=elf; request.elf_size=sizeof(elf);
    request.load_bias=0x100000u;
    request.buffers=buffers; request.buffer_count=2u;
    re_guest_memory_init(&memory);
    assert(re_guest_elf32_load_image(&memory,&request,&result,NULL,NULL)==
        RE_GUEST_ELF32_LOAD_E_FLAGS);
    assert(re_guest_memory_region_count(&memory)==0u);

    make_elf(elf, RE_GUEST_ELF32_PF_R | RE_GUEST_ELF32_PF_X);
    assert(re_guest_elf32_load_image(&memory,&request,&result,NULL,NULL)==
        RE_GUEST_ELF32_LOAD_E_BUFFER);
    assert(re_guest_memory_region_count(&memory)==0u);
    return 0;
}

int main(void) {
    assert(test_load_and_relocate()==0);
    assert(test_relocation_failure_rolls_back_maps()==0);
    assert(test_wx_and_short_buffer_rejected_before_mapping()==0);
    puts("PASS: integrated ELF32 ARM load/materialize/relocate/permission pipeline");
    return 0;
}
