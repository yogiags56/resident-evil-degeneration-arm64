#include "re_guest_cpu_bus_adapter.h"

void re_guest_cpu_bus_init(REGUESTCPUBUS *cpu_bus,
    const REGUESTMEMBUS *memory_bus) {
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
    return re_guest_memory_write8((REGUESTMEMBUS *)cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_write16(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint16_t value) {
    if (!cpu_bus || !cpu_bus->bus) return RE_GUEST_MEM_E_ARGUMENT;
    return re_guest_memory_write16((REGUESTMEMBUS *)cpu_bus->bus, address, value);
}

REGUESTMEMStatus re_guest_cpu_write32(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint32_t value) {
    if (!cpu_bus || !cpu_bus->bus) return RE_GUEST_MEM_E_ARGUMENT;
    return re_guest_memory_write32((REGUESTMEMBUS *)cpu_bus->bus, address, value);
}
