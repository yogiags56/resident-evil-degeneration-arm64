#include "re_guest_cpu_memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTCPUMEMORY cpu_memory;
    uint8_t code[8] = {0};
    uint8_t data[16] = {0};
    uint8_t b = 0xffu;
    uint16_t h = 0xffffu;
    uint32_t w = 0xffffffffu;

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory,0x1000u,sizeof(code),code,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory,0x2000u,sizeof(data),data,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus,&memory);
    re_guest_cpu_memory_init(&cpu_memory,&bus);

    assert(re_guest_cpu_mem_write8(&cpu_memory,0x2000u,0xabu)==RE_CPU_OK);
    assert(re_guest_cpu_mem_write16(&cpu_memory,0x2002u,0xcdefu)==RE_CPU_OK);
    assert(re_guest_cpu_mem_write32(&cpu_memory,0x2004u,0x12345678u)==RE_CPU_OK);
    assert(re_guest_cpu_mem_read8(&cpu_memory,0x2000u,&b)==RE_CPU_OK && b==0xabu);
    assert(re_guest_cpu_mem_read16(&cpu_memory,0x2002u,&h)==RE_CPU_OK && h==0xcdefu);
    assert(re_guest_cpu_mem_read32(&cpu_memory,0x2004u,&w)==RE_CPU_OK && w==0x12345678u);

    assert(re_guest_cpu_mem_write8(&cpu_memory,0x1000u,1u)==RE_CPU_E_PERMISSION);
    assert(re_guest_cpu_mem_read8(&cpu_memory,0x1000u,&b)==RE_CPU_OK && b==0u);
    assert(re_guest_cpu_mem_read8(&cpu_memory,0x3000u,&b)==RE_CPU_E_MEMORY && b==0u);
    assert(re_guest_cpu_mem_write32(&cpu_memory,0x2002u,0u)==RE_CPU_E_ALIGNMENT);
    assert(re_guest_cpu_mem_read16(&cpu_memory,0x2001u,&h)==RE_CPU_E_ALIGNMENT && h==0u);
    assert(re_guest_cpu_mem_read32(&cpu_memory,0x2000u,NULL)==RE_CPU_E_ARGUMENT);
    assert(re_guest_cpu_mem_write8(NULL,0x2000u,1u)==RE_CPU_E_ARGUMENT);

    puts("Guest CPU memory facade tests: PASS (14 assertions)");
    return 0;
}
