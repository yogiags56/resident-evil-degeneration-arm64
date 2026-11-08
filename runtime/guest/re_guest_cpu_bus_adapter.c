#include "re_guest_cpu_bus_adapter.h"

void re_guest_cpu_bus_init(REGUESTCPUBUS *cpu_bus,
    REGUESTMEMBUS *memory_bus) {
    if (cpu_bus) cpu_bus->bus = memory_bus;
}

REGUESTMEMStatus re_guest_cpu_fetch16(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint16_t *value) {
    if (!cpu_bus || !cpu_bus->bus) {
        if (value) *value = 0;
        return RE_GUEST_MEM_E_ARGUMENT;
    }
    return re_guest_memory_fetch16(cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_fetch_instruction(
    const REGUESTCPUBUS *cpu_bus, uint32_t address, int thumb_state,
    uint32_t *instruction, uint8_t *width) {
    uint16_t first = 0;
    uint16_t second = 0;
    uint32_t arm_word = 0;
    REGUESTMEMStatus status;

    if (instruction) *instruction = 0;
    if (width) *width = 0;
    if (!cpu_bus || !cpu_bus->bus || !instruction || !width)
        return RE_GUEST_MEM_E_ARGUMENT;

    if (!thumb_state) {
        status = re_guest_memory_fetch32(cpu_bus->bus, address, &arm_word);
        if (status != RE_GUEST_MEM_OK) return status;
        *instruction = arm_word;
        *width = 4u;
        return RE_GUEST_MEM_OK;
    }

    status = re_guest_memory_fetch16(cpu_bus->bus, address, &first);
    if (status != RE_GUEST_MEM_OK) return status;

    /* ARMv7 Thumb-2 32-bit instruction prefixes: 11101, 11110, 11111. */
    if ((first & 0xf800u) == 0xe800u ||
        (first & 0xf800u) == 0xf000u ||
        (first & 0xf800u) == 0xf800u) {
        if (address > UINT32_MAX - 2u) return RE_GUEST_MEM_E_RANGE;
        status = re_guest_memory_fetch16(cpu_bus->bus, address + 2u, &second);
        if (status != RE_GUEST_MEM_OK) return status;
        *instruction = ((uint32_t)first << 16) | second;
        *width = 4u;
        return RE_GUEST_MEM_OK;
    }

    *instruction = first;
    *width = 2u;
    return RE_GUEST_MEM_OK;
}

REGUESTMEMStatus re_guest_cpu_fetch32(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint32_t *value) {
    if (!cpu_bus || !cpu_bus->bus) {
        if (value) *value = 0;
        return RE_GUEST_MEM_E_ARGUMENT;
    }
    return re_guest_memory_fetch32(cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_read8(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint8_t *value) {
    if (!cpu_bus || !cpu_bus->bus) {
        if (value) *value = 0;
        return RE_GUEST_MEM_E_ARGUMENT;
    }
    return re_guest_memory_read8(cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_read16(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint16_t *value) {
    if (!cpu_bus || !cpu_bus->bus) {
        if (value) *value = 0;
        return RE_GUEST_MEM_E_ARGUMENT;
    }
    return re_guest_memory_read16(cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_read32(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint32_t *value) {
    if (!cpu_bus || !cpu_bus->bus) {
        if (value) *value = 0;
        return RE_GUEST_MEM_E_ARGUMENT;
    }
    return re_guest_memory_read32(cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_write8(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint8_t value) {
    if (!cpu_bus || !cpu_bus->bus) return RE_GUEST_MEM_E_ARGUMENT;
    return re_guest_memory_write8(cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_write16(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint16_t value) {
    if (!cpu_bus || !cpu_bus->bus) return RE_GUEST_MEM_E_ARGUMENT;
    return re_guest_memory_write16(cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_write32(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint32_t value) {
    if (!cpu_bus || !cpu_bus->bus) return RE_GUEST_MEM_E_ARGUMENT;
    return re_guest_memory_write32(cpu_bus->bus, address, value);
}
