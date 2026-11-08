#include "re_guest_cpu.h"
#include "re_guest_cpu_bus_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void setup_bus(REGUESTMEMORY *map, REGUESTMEMBUS *bus,
    REGUESTCPUBUS *cpu_bus, uint8_t *bytes, size_t size,
    uint32_t base, uint32_t permissions) {
    re_guest_memory_init(map);
    assert(re_guest_memory_map(map, base, size, bytes, permissions) ==
        RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(bus, map);
    re_guest_cpu_bus_init(cpu_bus, bus);
}

static void test_thumb16_and_fail_closed(void) {
    uint8_t code[8] = {0x09,0x20,0x03,0x30,0,0,0,0};
    uint8_t bus_code[8] = {0x09,0x20,0x03,0x30,0,0,0,0};
    REGUESTMEM legacy = {code,sizeof(code),0x1000u};
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUBUS cpu_bus;
    REGUESTCPU cpu;

    setup_bus(&map,&bus,&cpu_bus,bus_code,sizeof(bus_code),0x1000u,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC);

    re_guest_cpu_reset(&cpu,0x1000u,1);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[0]==9u && cpu.r[15]==0x1002u && cpu.steps==1u);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[0]==12u && cpu.r[15]==0x1004u && cpu.steps==2u);

    assert(re_guest_memory_unmap(&map,0x1000u)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&map,0x1000u,sizeof(bus_code),bus_code,
        RE_GUEST_MEM_READ)==RE_GUEST_MEM_OK);
    re_guest_cpu_reset(&cpu,0x1000u,1);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_E_PERMISSION);
    assert(cpu.steps==0u && cpu.r[0]==0u && cpu.r[15]==0x1000u);

    assert(re_guest_memory_unmap(&map,0x1000u)==RE_GUEST_MEM_OK);
    bus_code[0]=0x01;
    assert(re_guest_memory_map(&map,0x1000u,sizeof(bus_code),bus_code,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_E_FETCH_MISMATCH);
    assert(cpu.steps==0u && cpu.r[15]==0x1000u);
}

static void test_arm_checked_step(void) {
    /* ARM: MOV r0, #42 ; little-endian encoding 0xE3A0002A. */
    uint8_t code[4] = {0x2a,0x00,0xa0,0xe3};
    uint8_t bus_code[4] = {0x2a,0x00,0xa0,0xe3};
    REGUESTMEM legacy = {code,sizeof(code),0x2000u};
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUBUS cpu_bus;
    REGUESTCPU cpu;

    setup_bus(&map,&bus,&cpu_bus,bus_code,sizeof(bus_code),0x2000u,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC);
    re_guest_cpu_reset(&cpu,0x2000u,0);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[0]==42u);
    assert(cpu.r[15]==0x2004u);
    assert(cpu.steps==1u);
}

static void test_thumb2_movw_checked_step(void) {
    /* Thumb-2 MOVW r0, #42: halfwords F240 002A. */
    uint8_t code[4] = {0x40,0xf2,0x2a,0x00};
    uint8_t bus_code[4] = {0x40,0xf2,0x2a,0x00};
    REGUESTMEM legacy = {code,sizeof(code),0x3000u};
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUBUS cpu_bus;
    REGUESTCPU cpu;

    setup_bus(&map,&bus,&cpu_bus,bus_code,sizeof(bus_code),0x3000u,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC);
    re_guest_cpu_reset(&cpu,0x3000u,1);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[0]==42u);
    assert(cpu.r[15]==0x3004u);
    assert(cpu.steps==1u);
}

static void test_it_block_checked_steps(void) {
    /* IT EQ ; MOVS r1,#7. */
    uint8_t code[4] = {0x08,0xbf,0x07,0x21};
    uint8_t bus_code[4] = {0x08,0xbf,0x07,0x21};
    REGUESTMEM legacy = {code,sizeof(code),0x4000u};
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUBUS cpu_bus;
    REGUESTCPU cpu;

    setup_bus(&map,&bus,&cpu_bus,bus_code,sizeof(bus_code),0x4000u,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC);

    re_guest_cpu_reset(&cpu,0x4000u,1);
    cpu.cpsr |= RE_CPSR_Z;
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[15]==0x4002u && cpu.itstate==0x08u);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[1]==7u && cpu.r[15]==0x4004u);
    assert(cpu.itstate==0u && cpu.steps==2u);

    /* EQ fails: the instruction in the IT block is skipped, not executed. */
    re_guest_cpu_reset(&cpu,0x4000u,1);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.itstate==0x08u);
    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==RE_CPU_OK);
    assert(cpu.r[1]==0u && cpu.r[15]==0x4004u);
    assert(cpu.itstate==0u && cpu.steps==2u);
}

static void test_thumb2_second_halfword_permission(void) {
    uint8_t code[4] = {0x40,0xf2,0x2a,0x00};
    uint8_t first[2] = {0x40,0xf2};
    uint8_t second[2] = {0x2a,0x00};
    REGUESTMEM legacy = {code,sizeof(code),0x5000u};
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUBUS cpu_bus;
    REGUESTCPU cpu;

    re_guest_memory_init(&map);
    assert(re_guest_memory_map(&map,0x5000u,2u,first,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC)==RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&map,0x5002u,2u,second,
        RE_GUEST_MEM_READ)==RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus,&map);
    re_guest_cpu_bus_init(&cpu_bus,&bus);
    re_guest_cpu_reset(&cpu,0x5000u,1);

    assert(re_guest_cpu_step_checked(&cpu,&legacy,&cpu_bus)==
        RE_CPU_E_PERMISSION);
    assert(cpu.r[15]==0x5000u && cpu.steps==0u && cpu.r[0]==0u);
}

int main(void) {
    REGUESTCPU cpu;
    uint8_t code[4] = {0};
    REGUESTMEM legacy = {code,sizeof(code),0x6000u};
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUBUS cpu_bus;

    test_thumb16_and_fail_closed();
    test_arm_checked_step();
    test_thumb2_movw_checked_step();
    test_it_block_checked_steps();
    test_thumb2_second_halfword_permission();

    setup_bus(&map,&bus,&cpu_bus,code,sizeof(code),0x6000u,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_EXEC);
    re_guest_cpu_reset(&cpu,0x6000u,1);
    assert(re_guest_cpu_step_checked(NULL,&legacy,&cpu_bus)==RE_CPU_E_ARGUMENT);

    puts("Guest CPU checked-step tests: PASS (ARM, Thumb, Thumb-2, IT, permission)");
    return 0;
}
