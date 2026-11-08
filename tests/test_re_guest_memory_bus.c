#include "re_guest_memory_bus.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    uint8_t code[16] = {0x12, 0x34, 0x56, 0x78};
    uint8_t read_only[8] = {0xaa};
    uint8_t writable[8] = {0};
    uint8_t byte_value = 0;
    uint16_t half_value = 0;
    uint32_t word_value = 0;

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory, 0x1000u, sizeof(code), code,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_EXEC) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory, 0x2000u, sizeof(read_only), read_only,
        RE_GUEST_MEM_READ) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory, 0x3000u, sizeof(writable), writable,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);

    assert(re_guest_memory_fetch8(&bus, 0x1000u, &byte_value) == RE_GUEST_MEM_OK && byte_value == 0x12u);
    assert(re_guest_memory_fetch16(&bus, 0x1000u, &half_value) == RE_GUEST_MEM_OK && half_value == 0x3412u);
    assert(re_guest_memory_fetch32(&bus, 0x1000u, &word_value) == RE_GUEST_MEM_OK && word_value == 0x78563412u);
    assert(re_guest_memory_read8(&bus, 0x2000u, &byte_value) == RE_GUEST_MEM_OK && byte_value == 0xaau);
    assert(re_guest_memory_fetch8(&bus, 0x2000u, &byte_value) == RE_GUEST_MEM_E_PERMISSION && byte_value == 0u);
    assert(re_guest_memory_write8(&bus, 0x2000u, 1u) == RE_GUEST_MEM_E_PERMISSION);
    assert(re_guest_memory_write32(&bus, 0x3000u, 0xdeadbeefu) == RE_GUEST_MEM_OK);
    assert(writable[0] == 0xefu && writable[1] == 0xbeu && writable[2] == 0xadu && writable[3] == 0xdeu);
    assert(re_guest_memory_read32(&bus, 0x3000u, &word_value) == RE_GUEST_MEM_OK && word_value == 0xdeadbeefu);

    assert(re_guest_memory_read8(&bus, 0x1800u, &byte_value) == RE_GUEST_MEM_E_NOT_FOUND);
    assert(re_guest_memory_read32(&bus, 0x3006u, &word_value) == RE_GUEST_MEM_E_RANGE);
    assert(re_guest_memory_read16(&bus, 0x3001u, &half_value) == RE_GUEST_MEM_E_RANGE && half_value == 0u);
    assert(re_guest_memory_write32(&bus, 0x3002u, 0u) == RE_GUEST_MEM_E_RANGE);
    assert(re_guest_memory_fetch32(&bus, 0x100eu, &word_value) == RE_GUEST_MEM_E_NOT_FOUND);
    assert(re_guest_memory_fetch16(&bus, 0x100fu, &half_value) == RE_GUEST_MEM_E_RANGE && half_value == 0u);
    assert(re_guest_memory_read8(&bus, 0xffffffffu, &byte_value) == RE_GUEST_MEM_E_NOT_FOUND);
    assert(re_guest_memory_fetch8(&bus, 0x1000u, NULL) == RE_GUEST_MEM_E_ARGUMENT);

    puts("Guest memory bus tests: PASS (17 assertions)");
    return 0;
}
