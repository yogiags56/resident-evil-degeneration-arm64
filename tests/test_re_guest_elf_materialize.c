#include "re_guest_elf_materialize.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    REGUESTMEMORY memory;
    uint8_t elf[64], text[32], data[32], readonly[16];
    REGUESTELFSEGMENT segments[2];
    size_t failed = 999;

    memset(elf, 0xA5, sizeof(elf));
    memset(text, 0xCC, sizeof(text));
    memset(data, 0xDD, sizeof(data));
    memset(readonly, 0xEE, sizeof(readonly));
    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory, 0x1000, sizeof(text), text,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory, 0x2000, sizeof(data), data,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory, 0x3000, sizeof(readonly), readonly,
        RE_GUEST_MEM_READ) == RE_GUEST_MEM_OK);

    segments[0] = (REGUESTELFSEGMENT){0x1000, 4, 8, 16};
    segments[1] = (REGUESTELFSEGMENT){0x2000, 20, 12, 20};
    assert(re_guest_elf_materialize(&memory, elf, sizeof(elf), segments, 2,
        &failed) == RE_GUEST_ELF_MATERIALIZE_OK);
    assert(failed == 2);
    assert(memcmp(text, elf + 4, 8) == 0);
    for (size_t i = 8; i < 16; ++i) assert(text[i] == 0);
    assert(memcmp(data, elf + 20, 12) == 0);
    for (size_t i = 12; i < 20; ++i) assert(data[i] == 0);

    /* Later invalid source range must not partially modify the first segment. */
    memset(text, 0xCC, sizeof(text));
    memset(data, 0xDD, sizeof(data));
    segments[0] = (REGUESTELFSEGMENT){0x1000, 0, 8, 16};
    segments[1] = (REGUESTELFSEGMENT){0x2000, 60, 8, 16};
    assert(re_guest_elf_materialize(&memory, elf, sizeof(elf), segments, 2,
        &failed) == RE_GUEST_ELF_MATERIALIZE_E_SOURCE_RANGE);
    assert(failed == 1);
    for (size_t i = 0; i < sizeof(text); ++i) assert(text[i] == 0xCC);
    for (size_t i = 0; i < sizeof(data); ++i) assert(data[i] == 0xDD);

    segments[1] = (REGUESTELFSEGMENT){0x2000, 0, 17, 16};
    assert(re_guest_elf_materialize(&memory, elf, sizeof(elf), segments, 2,
        &failed) == RE_GUEST_ELF_MATERIALIZE_E_SIZE);
    assert(failed == 1);

    segments[0] = (REGUESTELFSEGMENT){0x1000, 0, 8, 16};
    segments[1] = (REGUESTELFSEGMENT){0x1008, 8, 8, 16};
    assert(re_guest_elf_materialize(&memory, elf, sizeof(elf), segments, 2,
        &failed) == RE_GUEST_ELF_MATERIALIZE_E_OVERLAP);
    assert(failed == 1);

    segments[0] = (REGUESTELFSEGMENT){0x1000, 0, 8, 16};
    segments[1] = (REGUESTELFSEGMENT){0x3000, 8, 8, 16};
    assert(re_guest_elf_materialize(&memory, elf, sizeof(elf), segments, 2,
        &failed) == RE_GUEST_ELF_MATERIALIZE_E_GUEST_RANGE);
    assert(failed == 1);

    segments[0] = (REGUESTELFSEGMENT){0xFFFFFFF8u, 0, 4, 16};
    assert(re_guest_elf_materialize(&memory, elf, sizeof(elf), segments, 1,
        &failed) == RE_GUEST_ELF_MATERIALIZE_E_GUEST_RANGE);
    assert(failed == 0);

    segments[0] = (REGUESTELFSEGMENT){0x3000, 0, 8, 16};
    assert(re_guest_elf_materialize(&memory, elf, sizeof(elf), segments, 1,
        &failed) == RE_GUEST_ELF_MATERIALIZE_E_PERMISSION);
    assert(failed == 0);
    for (size_t i = 0; i < sizeof(readonly); ++i) assert(readonly[i] == 0xEE);

    assert(re_guest_elf_materialize(&memory, NULL, 0, NULL, 0, &failed)
        == RE_GUEST_ELF_MATERIALIZE_OK);
    puts("Guest ELF materialization tests: PASS");
    return 0;
}
