#include "re_guest_cpu_bus_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    REGUESTMEMORY memory;
    REGUESTMEMBUS memory_bus;
    REGUESTCPUBUS cpu_bus;
    uint8_t code[8] = {0x00, 0xbf, 0x2a, 0x00, 0, 0, 0, 0};
    uint8_t data[8] = {0x78, 0x56, 0x34, 0x12};
    uint8_t ro[4] = {0xa5, 0, 0, 0};
    uint16_t half = 0;
    uint32_t word = 0;
    uint8_t byte = 0;

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory, 0x1000u, sizeof(code), code,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_EXEC) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory, 0x2000u, sizeof(data), data,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory, 0x3000u, sizeof(ro), ro,
        RE_GUEST_MEM_READ) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&memory_bus, &memory);
    re_guest_cpu_bus_init(&cpu_bus, &memory_bus);

    assert(re_guest_cpu_fetch16(&cpu_bus, 0x1000u, &half) == RE_GUEST_MEM_OK);
    assert(half == 0xbf00u);
    assert(re_guest_cpu_fetch32(&cpu_bus, 0x1000u, &word) == RE_GUEST_MEM_OK);
    assert(word == 0x002abf00u);
    assert(re_guest_cpu_fetch16(&cpu_bus, 0x2000u, &half) == RE_GUEST_MEM_E_PERMISSION);
    assert(half == 0u);

    assert(re_guest_cpu_read8(&cpu_bus, 0x2000u, &byte) == RE_GUEST_MEM_OK);
    assert(byte == 0x78u);
    assert(re_guest_cpu_read16(&cpu_bus, 0x2000u, &half) == RE_GUEST_MEM_OK);
    assert(half == 0x5678u);
    assert(re_guest_cpu_read32(&cpu_bus, 0x2000u, &word) == RE_GUEST_MEM_OK);
    assert(word == 0x12345678u);

    assert(re_guest_cpu_write8(&cpu_bus, 0x2004u, 0xabu) == RE_GUEST_MEM_OK);
    assert(data[4] == 0xabu);
    assert(re_guest_cpu_write16(&cpu_bus, 0x2004u, 0xcdefu) == RE_GUEST_MEM_OK);
    assert(data[4] == 0xefu && data[5] == 0xcdu);
    assert(re_guest_cpu_write32(&cpu_bus, 0x2004u, 0xdeadbeefu) == RE_GUEST_MEM_OK);
    assert(data[4] == 0xefu && data[7] == 0xdeu);

    assert(re_guest_cpu_read8(&cpu_bus, 0x3000u, &byte) == RE_GUEST_MEM_OK);
    assert(byte == 0xa5u);
    assert(re_guest_cpu_write8(&cpu_bus, 0x3000u, 0u) == RE_GUEST_MEM_E_PERMISSION);
    assert(re_guest_cpu_read8(&cpu_bus, 0x1800u, &byte) == RE_GUEST_MEM_E_NOT_FOUND);
    assert(re_guest_cpu_fetch16(&cpu_bus, 0x1001u, &half) == RE_GUEST_MEM_E_RANGE);
    assert(re_guest_cpu_fetch16(NULL, 0x1000u, &half) == RE_GUEST_MEM_E_ARGUMENT);

    puts("Guest CPU bus adapter tests: PASS (20 assertions)");
    return 0;
}
