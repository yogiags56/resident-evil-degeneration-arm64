#include "re_guest_cpu_bus_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTCPUBUS cpu_bus;
    uint8_t code[8] = {0x00,0xbf,0x00,0xf0,0x00,0x80,0,0};
    uint8_t data[4] = {0};
    uint32_t instruction = 0xfeedfaceu;
    uint8_t width = 0xffu;

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory,0x1000u,sizeof(code),code,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory,0x2000u,sizeof(data),data,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus,&memory);
    re_guest_cpu_bus_init(&cpu_bus,&bus);

    assert(re_guest_cpu_fetch_instruction(&cpu_bus,0x1000u,1,
        &instruction,&width)==RE_GUEST_MEM_OK);
    assert(instruction==0xbf00u && width==2u);
    assert(re_guest_cpu_fetch_instruction(&cpu_bus,0x1002u,1,
        &instruction,&width)==RE_GUEST_MEM_OK);
    assert(instruction==0xf0008000u && width==4u);
    assert(re_guest_cpu_fetch_instruction(&cpu_bus,0x1000u,0,
        &instruction,&width)==RE_GUEST_MEM_OK);
    assert(instruction==0xf000bf00u && width==4u);

    instruction=0xfeedfaceu; width=0xffu;
    assert(re_guest_cpu_fetch_instruction(&cpu_bus,0x2000u,1,
        &instruction,&width)==RE_GUEST_MEM_E_PERMISSION);
    assert(instruction==0u && width==0u);

    /* First Thumb halfword is executable; second is in a non-executable region. */
    assert(re_guest_memory_unmap(&memory,0x1000u)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory,0x1000u,2u,code,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory,0x1002u,2u,code+2,
        RE_GUEST_MEM_READ)==RE_GUEST_MEM_OK);
    assert(re_guest_cpu_fetch_instruction(&cpu_bus,0x1000u,1,
        &instruction,&width)==RE_GUEST_MEM_E_PERMISSION);
    assert(instruction==0u && width==0u);

    assert(re_guest_cpu_fetch_instruction(&cpu_bus,0x1000u,1,
        NULL,&width)==RE_GUEST_MEM_E_ARGUMENT);
    assert(re_guest_cpu_fetch_instruction(NULL,0x1000u,1,
        &instruction,&width)==RE_GUEST_MEM_E_ARGUMENT);

    puts("Guest CPU instruction fetch tests: PASS");
    return 0;
}
