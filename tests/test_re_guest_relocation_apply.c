#include "re_guest_relocation_apply.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void put32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}

int main(void) {
    REGUESTMEMORY memory;
    uint8_t image[64];
    size_t failed = 999;
    REGUESTRELOCATION relocs[4];

    memset(image, 0, sizeof(image));
    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory, 0x1000, sizeof(image), image,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);

    put32(image + 0, 0x24);
    put32(image + 4, 0x10);
    relocs[0] = (REGUESTRELOCATION){0x1000, RE_ARM_R_RELATIVE, 0, 0,
        0x4000, 0, 0, 0}; /* REL: B + *(P) */
    relocs[1] = (REGUESTRELOCATION){0x1004, RE_ARM_R_ABS32, 3, 0x2000,
        0, 0, 0, 1}; /* REL: S + *(P) */
    relocs[2] = (REGUESTRELOCATION){0x1008, RE_ARM_R_GLOB_DAT, 4, 0x3000,
        0, 0, 1, 1};
    relocs[3] = (REGUESTRELOCATION){0x100C, RE_ARM_R_JUMP_SLOT, 5, 0x5000,
        0, 0, 1, 1};
    assert(re_guest_relocations_apply(&memory, relocs, 4, &failed)
        == RE_GUEST_RELOC_APPLY_OK);
    assert(failed == 4);
    assert(get32(image + 0) == 0x4024);
    assert(get32(image + 4) == 0x2010);
    assert(get32(image + 8) == 0x3000);
    assert(get32(image + 12) == 0x5000);

    /* RELA uses its explicit addend and does not read target contents. */
    put32(image + 16, 0xDEADBEEF);
    relocs[0] = (REGUESTRELOCATION){0x1010, RE_ARM_R_ABS32, 7, 0x100,
        0, 0x20, 1, 1};
    assert(re_guest_relocations_apply(&memory, relocs, 1, &failed)
        == RE_GUEST_RELOC_APPLY_OK);
    assert(get32(image + 16) == 0x120);

    /* Invalid last relocation leaves earlier target bytes unchanged. */
    put32(image + 20, 0x11111111);
    put32(image + 24, 0x22222222);
    relocs[0] = (REGUESTRELOCATION){0x1014, RE_ARM_R_RELATIVE, 0, 0,
        0x1000, 4, 1, 0};
    relocs[1] = (REGUESTRELOCATION){0x1018, 0x7f, 0, 0,
        0, 0, 1, 0};
    assert(re_guest_relocations_apply(&memory, relocs, 2, &failed)
        == RE_GUEST_RELOC_APPLY_E_UNSUPPORTED);
    assert(failed == 1);
    assert(get32(image + 20) == 0x11111111);
    assert(get32(image + 24) == 0x22222222);

    relocs[0] = (REGUESTRELOCATION){0x1014, RE_ARM_R_RELATIVE, 1, 0,
        0, 0, 1, 0};
    assert(re_guest_relocations_apply(&memory, relocs, 1, &failed)
        == RE_GUEST_RELOC_APPLY_E_RELATIVE_SYMBOL);
    assert(get32(image + 20) == 0x11111111);

    relocs[0] = (REGUESTRELOCATION){0x1014, RE_ARM_R_GLOB_DAT, 1, 0,
        0, 0, 1, 0};
    assert(re_guest_relocations_apply(&memory, relocs, 1, &failed)
        == RE_GUEST_RELOC_APPLY_E_SYMBOL);

    relocs[0] = (REGUESTRELOCATION){0x1015, RE_ARM_R_RELATIVE, 0, 0,
        0, 0, 1, 0};
    assert(re_guest_relocations_apply(&memory, relocs, 1, &failed)
        == RE_GUEST_RELOC_APPLY_E_ALIGNMENT);

    relocs[0] = (REGUESTRELOCATION){0x1014, RE_ARM_R_RELATIVE, 0, 0,
        0, 0, 1, 0};
    relocs[1] = relocs[0];
    assert(re_guest_relocations_apply(&memory, relocs, 2, &failed)
        == RE_GUEST_RELOC_APPLY_E_DUPLICATE);
    assert(failed == 1);

    assert(re_guest_relocations_apply(&memory, NULL, 0, &failed)
        == RE_GUEST_RELOC_APPLY_OK);
    assert(failed == 0);
    puts("Guest relocation application tests: PASS");
    return 0;
}
