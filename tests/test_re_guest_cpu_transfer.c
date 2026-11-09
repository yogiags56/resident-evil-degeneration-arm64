#include "re_guest_cpu_transfer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void setup_bus(REGUESTMEMORY *map, REGUESTMEMBUS *bus,
    REGUESTCPUMEMORY *memory, uint8_t *data, size_t size) {
    re_guest_memory_init(map);
    assert(re_guest_memory_map(map, 0x8000u, size, data,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(bus, map);
    re_guest_cpu_memory_init(memory, bus);
}

static void differential_case(uint32_t instruction, uint32_t base,
    uint32_t source, uint32_t initial_word) {
    uint8_t legacy_data[0x7020] = {0};
    uint8_t bus_data[32] = {0};
    REGUESTMEM legacy_memory;
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUMEMORY memory;
    REGUESTCPU legacy_cpu, helper_cpu;
    unsigned rd = (instruction >> 12) & 15u;

    legacy_data[0] = (uint8_t)instruction;
    legacy_data[1] = (uint8_t)(instruction >> 8);
    legacy_data[2] = (uint8_t)(instruction >> 16);
    legacy_data[3] = (uint8_t)(instruction >> 24);
    legacy_data[0x7000] = (uint8_t)initial_word;
    legacy_data[0x7001] = (uint8_t)(initial_word >> 8);
    legacy_data[0x7002] = (uint8_t)(initial_word >> 16);
    legacy_data[0x7003] = (uint8_t)(initial_word >> 24);
    memcpy(bus_data, legacy_data + 0x7000, sizeof(bus_data));

    legacy_memory.bytes = legacy_data;
    legacy_memory.size = sizeof(legacy_data);
    legacy_memory.guest_base = 0x1000u;

    setup_bus(&map, &bus, &memory, bus_data, sizeof(bus_data));
    re_guest_cpu_reset(&legacy_cpu, 0x1000u, 0);
    re_guest_cpu_reset(&helper_cpu, 0x1000u, 0);
    legacy_cpu.r[1] = helper_cpu.r[1] = base;
    legacy_cpu.r[rd] = helper_cpu.r[rd] = source;

    REGUESTCPUStatus legacy_status = re_guest_cpu_step(&legacy_cpu, &legacy_memory);
    if (legacy_status != RE_CPU_OK) {
        fprintf(stderr, "legacy decoder failed instruction=0x%08x status=%d\n",
            instruction, (int)legacy_status);
    }
    assert(legacy_status == RE_CPU_OK);
    REGUESTCPUStatus helper_status = re_guest_cpu_arm_transfer_imm(
        &helper_cpu, &memory, instruction);
    if (helper_status != RE_CPU_OK) {
        fprintf(stderr, "helper failed instruction=0x%08x status=%d\n",
            instruction, (int)helper_status);
    }
    assert(helper_status == RE_CPU_OK);

    assert(legacy_cpu.r[rd] == helper_cpu.r[rd]);
    assert(memcmp(legacy_data + 0x7000, bus_data, sizeof(bus_data)) == 0);
}

int main(void) {
    REGUESTCPU cpu;
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUMEMORY memory;
    uint8_t data[32] = {0};

    setup_bus(&map, &bus, &memory, data, sizeof(data));
    re_guest_cpu_reset(&cpu, 0x1000u, 0);
    cpu.r[1] = 0x8000u;

    /* LDR r0,[r1] */
    data[0] = 0x78;
    data[1] = 0x56;
    data[2] = 0x34;
    data[3] = 0x12;
    assert(re_guest_cpu_arm_transfer_imm(
        &cpu, &memory, 0xe5910000u) == RE_CPU_OK);
    assert(cpu.r[0] == 0x12345678u);

    /* STR r0,[r1,#4] */
    assert(re_guest_cpu_arm_transfer_imm(
        &cpu, &memory, 0xe5810004u) == RE_CPU_OK);
    assert(data[4] == 0x78 && data[5] == 0x56 &&
        data[6] == 0x34 && data[7] == 0x12);

    /* LDRB r2,[r1] */
    assert(re_guest_cpu_arm_transfer_imm(
        &cpu, &memory, 0xe5d12000u) == RE_CPU_OK);
    assert(cpu.r[2] == 0x78u);

    /* STRB r2,[r1,#1] */
    cpu.r[2] = 0xa5u;
    assert(re_guest_cpu_arm_transfer_imm(
        &cpu, &memory, 0xe5c12001u) == RE_CPU_OK);
    assert(data[1] == 0xa5u);

    /* Differential parity against the legacy interpreter. */
    differential_case(0xe5910000u, 0x8000u, 0u, 0x12345678u);
    differential_case(0xe5910004u, 0x8000u, 0u, 0x12345678u);
    differential_case(0xe5110004u, 0x8004u, 0u, 0x12345678u);
    differential_case(0xe5810004u, 0x8000u, 0xa1b2c3d4u, 0u);
    differential_case(0xe5410001u, 0x8004u, 0xa1b2c3d4u, 0u);
    differential_case(0xe5d10002u, 0x8000u, 0u, 0x78563412u);
    differential_case(0xe5c10003u, 0x8000u, 0xa1b2c3d4u, 0u);

    /* Failed loads do not commit the destination register. */
    cpu.r[3] = 0xfeedfaceu;
    cpu.r[1] = 0x9000u;
    assert(re_guest_cpu_arm_transfer_imm(
        &cpu, &memory, 0xe5913000u) == RE_CPU_E_MEMORY);
    assert(cpu.r[3] == 0xfeedfaceu);

    /* Unsupported forms fail closed. */
    assert(re_guest_cpu_arm_transfer_imm(
        &cpu, &memory, 0xe7910002u) == RE_CPU_E_UNIMPLEMENTED);
    assert(re_guest_cpu_arm_transfer_imm(
        &cpu, &memory, 0xe5b10000u) == RE_CPU_E_UNIMPLEMENTED);
    assert(re_guest_cpu_arm_transfer_imm(
        NULL, &memory, 0xe5910000u) == RE_CPU_E_ARGUMENT);

    puts("A32 load/store helper tests: PASS (7 differential cases)");
    return 0;
}
