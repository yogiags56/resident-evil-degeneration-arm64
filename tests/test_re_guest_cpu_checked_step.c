#include "re_guest_cpu.h"
#include "re_guest_cpu_bus_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    uint8_t code[8] = {0x09,0x20,0x03,0x30,0,0,0,0};
    uint8_t bus_code[8] = {0x09,0x20,0x03,0x30,0,0,0,0};
    REGUESTMEM legacy = {code,sizeof(code),0x1000u};
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUBUS cpu_bus;
    REGUESTCPU cpu;

    re_guest_memory_init(&map);
    assert(re_guest_memory_map(&map,0x1000u,sizeof(bus_code),bus_code,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus,&map);
    re_guest_cpu_bus_init(&cpu_bus,&bus);

    re_guest_cpu_reset(&cpu,0x1000u,1);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[0]==9u && cpu.r[15]==0x1002u && cpu.steps==1u);

    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[0]==12u && cpu.r[15]==0x1004u && cpu.steps==2u);

    /* Permission is checked before legacy interpreter execution. */
    assert(re_guest_memory_unmap(&map,0x1000u)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&map,0x1000u,sizeof(bus_code),bus_code,
        RE_GUEST_MEM_READ)==RE_GUEST_MEM_OK);
    re_guest_cpu_reset(&cpu,0x1000u,1);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_E_PERMISSION);
    assert(cpu.steps==0u && cpu.r[0]==0u && cpu.r[15]==0x1000u);

    /* A bus image that differs from the legacy decoder view is rejected. */
    assert(re_guest_memory_unmap(&map,0x1000u)==RE_GUEST_MEM_OK);
    bus_code[0]=0x01;
    assert(re_guest_memory_map(&map,0x1000u,sizeof(bus_code),bus_code,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_E_FETCH_MISMATCH);
    assert(cpu.steps==0u && cpu.r[15]==0x1000u);

    assert(re_guest_cpu_step_checked(NULL,&legacy,&cpu_bus)==RE_CPU_E_ARGUMENT);
    puts("Guest CPU checked-step tests: PASS");
    return 0;
}
