#include "re_guest_cpu_transfer.h"

REGUESTCPUStatus re_guest_cpu_arm_transfer_imm(
    REGUESTCPU *cpu, const REGUESTCPUMEMORY *memory, uint32_t instruction) {
    unsigned rn, rd;
    uint32_t base, offset, address;
    int load, byte;
    REGUESTCPUStatus status;

    if (!cpu || !memory) return RE_CPU_E_ARGUMENT;

    /* A32 single data transfer, immediate offset, pre-index, no writeback. */
    if ((instruction & 0x0f7f0000u) != 0x051f0000u)
        return RE_CPU_E_UNIMPLEMENTED;

    /* Only AL condition is enabled in this first isolated migration step. */
    if ((instruction >> 28) != 0xeu)
        return RE_CPU_E_UNIMPLEMENTED;

    rn = (instruction >> 16) & 15u;
    rd = (instruction >> 12) & 15u;
    if (rn == 15u || rd == 15u) return RE_CPU_E_UNIMPLEMENTED;

    load = !!(instruction & (1u << 20));
    byte = !!(instruction & (1u << 22));
    base = cpu->r[rn];
    offset = instruction & 0x0fffu;
    address = (instruction & (1u << 23)) ? base + offset : base - offset;

    if (load) {
        uint32_t result = 0;
        if (byte) {
            uint8_t value = 0;
            status = re_guest_cpu_mem_read8(memory, address, &value);
            result = value;
        } else {
            status = re_guest_cpu_mem_read32(memory, address, &result);
        }
        if (status != RE_CPU_OK) return status;
        cpu->r[rd] = result;
        return RE_CPU_OK;
    }

    if (byte)
        return re_guest_cpu_mem_write8(memory, address,
            (uint8_t)cpu->r[rd]);

    return re_guest_cpu_mem_write32(memory, address, cpu->r[rd]);
}
