#include "re_guest_cpu_transfer.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    REGUESTCPU cpu;
    REGUESTMEMORY map;
    REGUESTMEMBUS bus;
    REGUESTCPUMEMORY memory;
    uint8_t data[32] = {0};

    re_guest_memory_init(&map);
    assert(re_guest_memory_map(&map,0x8000u,sizeof(data),data,
        RE_GUEST_MEM_READ|RE_GUEST_MEM_WRITE)==RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus,&map);
    re_guest_cpu_memory_init(&memory,&bus);
    re_guest_cpu_reset(&cpu,0x1000u,0);
    cpu.r[1]=0x8000u;

    /* LDR r0,[r1] */
    data[0]=0x78; data[1]=0x56; data[2]=0x34; data[3]=0x12;
    assert(re_guest_cpu_arm_transfer_imm(&cpu,&memory,0xe5910000u)==RE_CPU_OK);
    assert(cpu.r[0]==0x12345678u);

    /* STR r0,[r1,#4] */
    assert(re_guest_cpu_arm_transfer_imm(&cpu,&memory,0xe5810004u)==RE_CPU_OK);
    assert(data[4]==0x78 && data[5]==0x56 &&
        data[6]==0x34 && data[7]==0x12);

    /* LDRB r2,[r1] */
    assert(re_guest_cpu_arm_transfer_imm(&cpu,&memory,0xe5d12000u)==RE_CPU_OK);
    assert(cpu.r[2]==0x78u);

    /* STRB r2,[r1,#1] */
    cpu.r[2]=0xa5u;
    assert(re_guest_cpu_arm_transfer_imm(&cpu,&memory,0xe5c12001u)==RE_CPU_OK);
    assert(data[1]==0xa5u);

    /* Faults must not modify destination registers. */
    cpu.r[3]=0xfeedfaceu;
    assert(re_guest_cpu_arm_transfer_imm(&cpu,&memory,0xe5913000u)==RE_CPU_OK);
    assert(cpu.r[3]==0x12345678u);
    cpu.r[3]=0xfeedfaceu;
    cpu.r[1]=0x9000u;
    assert(re_guest_cpu_arm_transfer_imm(&cpu,&memory,0xe5913000u)==RE_CPU_E_MEMORY);
    assert(cpu.r[3]==0xfeedfaceu);

    /* Unsupported register-offset and writeback forms fail closed. */
    assert(re_guest_cpu_arm_transfer_imm(&cpu,&memory,0xe7910002u)==RE_CPU_E_UNIMPLEMENTED);
    assert(re_guest_cpu_arm_transfer_imm(&cpu,&memory,0xe5b10000u)==RE_CPU_E_UNIMPLEMENTED);
    assert(re_guest_cpu_arm_transfer_imm(NULL,&memory,0xe5910000u)==RE_CPU_E_ARGUMENT);

    puts("A32 isolated load/store transfer tests: PASS");
    return 0;
}
