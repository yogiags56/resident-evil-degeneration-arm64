#include "re_guest_cpu_memory.h"

static REGUESTCPUStatus map_status(REGUESTMEMStatus status) {
    switch (status) {
    case RE_GUEST_MEM_OK:
        return RE_CPU_OK;
    case RE_GUEST_MEM_E_PERMISSION:
        return RE_CPU_E_PERMISSION;
    case RE_GUEST_MEM_E_RANGE:
        return RE_CPU_E_ALIGNMENT;
    case RE_GUEST_MEM_E_ARGUMENT:
        return RE_CPU_E_ARGUMENT;
    case RE_GUEST_MEM_E_NOT_FOUND:
    case RE_GUEST_MEM_E_CAPACITY:
    case RE_GUEST_MEM_E_OVERLAP:
    default:
        return RE_CPU_E_MEMORY;
    }
}

void re_guest_cpu_memory_init(REGUESTCPUMEMORY *cpu_memory,
    REGUESTMEMBUS *memory_bus) {
    if (cpu_memory) cpu_memory->bus = memory_bus;
}

REGUESTCPUStatus re_guest_cpu_mem_read8(const REGUESTCPUMEMORY *m,
    uint32_t address, uint8_t *value) {
    REGUESTMEMStatus status;
    if (value) *value = 0;
    if (!m || !m->bus || !value) return RE_CPU_E_ARGUMENT;
    status = re_guest_memory_read8(m->bus, address, value);
    return map_status(status);
}

REGUESTCPUStatus re_guest_cpu_mem_read16(const REGUESTCPUMEMORY *m,
    uint32_t address, uint16_t *value) {
    REGUESTMEMStatus status;
    if (value) *value = 0;
    if (!m || !m->bus || !value) return RE_CPU_E_ARGUMENT;
    status = re_guest_memory_read16(m->bus, address, value);
    return map_status(status);
}

REGUESTCPUStatus re_guest_cpu_mem_read32(const REGUESTCPUMEMORY *m,
    uint32_t address, uint32_t *value) {
    REGUESTMEMStatus status;
    if (value) *value = 0;
    if (!m || !m->bus || !value) return RE_CPU_E_ARGUMENT;
    status = re_guest_memory_read32(m->bus, address, value);
    return map_status(status);
}

REGUESTCPUStatus re_guest_cpu_mem_write8(const REGUESTCPUMEMORY *m,
    uint32_t address, uint8_t value) {
    if (!m || !m->bus) return RE_CPU_E_ARGUMENT;
    return map_status(re_guest_memory_write8(m->bus, address, value));
}

REGUESTCPUStatus re_guest_cpu_mem_write16(const REGUESTCPUMEMORY *m,
    uint32_t address, uint16_t value) {
    if (!m || !m->bus) return RE_CPU_E_ARGUMENT;
    return map_status(re_guest_memory_write16(m->bus, address, value));
}

REGUESTCPUStatus re_guest_cpu_mem_write32(const REGUESTCPUMEMORY *m,
    uint32_t address, uint32_t value) {
    if (!m || !m->bus) return RE_CPU_E_ARGUMENT;
    return map_status(re_guest_memory_write32(m->bus, address, value));
}
