#include "re_guest_cpu.h"
#include "re_guest_cpu_bus_adapter.h"

/*
 * Transitional checked entry point.
 * The bus is authoritative for EXEC permission and instruction bytes must
 * match the legacy contiguous view before the existing decoder is entered.
 * Data accesses are deliberately unchanged in this stage.
 */
REGUESTCPUStatus re_guest_cpu_step_checked(REGUESTCPU *cpu,
    const REGUESTMEM *legacy_memory, const REGUESTCPUBUS *cpu_bus) {
    uint32_t fetched = 0;
    uint8_t width = 0;
    size_t offset = 0;
    uint16_t legacy_first = 0;
    uint16_t legacy_second = 0;
    REGUESTMEMStatus ms;

    if (!cpu || !legacy_memory || !legacy_memory->bytes ||
        !cpu_bus || !cpu_bus->bus)
        return RE_CPU_E_ARGUMENT;

    ms = re_guest_cpu_fetch_instruction(cpu_bus, cpu->r[15],
        cpu->thumb, &fetched, &width);

    if (ms == RE_GUEST_MEM_E_PERMISSION) return RE_CPU_E_PERMISSION;
    if (ms == RE_GUEST_MEM_E_RANGE || ms == RE_GUEST_MEM_E_ARGUMENT)
        return RE_CPU_E_ALIGNMENT;
    if (ms != RE_GUEST_MEM_OK) return RE_CPU_E_MEMORY;

    if (cpu->r[15] < legacy_memory->guest_base)
        return RE_CPU_E_MEMORY;

    offset = (size_t)(cpu->r[15] - legacy_memory->guest_base);
    if (offset > legacy_memory->size ||
        (size_t)width > legacy_memory->size - offset)
        return RE_CPU_E_MEMORY;

    if (cpu->thumb) {
        legacy_first = (uint16_t)(legacy_memory->bytes[offset] |
            ((uint16_t)legacy_memory->bytes[offset + 1u] << 8));
        if (legacy_first != (uint16_t)(fetched >> (width == 4u ? 16u : 0u)))
            return RE_CPU_E_FETCH_MISMATCH;
        if (width == 4u) {
            legacy_second = (uint16_t)(legacy_memory->bytes[offset + 2u] |
                ((uint16_t)legacy_memory->bytes[offset + 3u] << 8));
            if (legacy_second != (uint16_t)fetched)
                return RE_CPU_E_FETCH_MISMATCH;
        }
    } else {
        uint32_t legacy_word = (uint32_t)legacy_memory->bytes[offset] |
            ((uint32_t)legacy_memory->bytes[offset + 1u] << 8) |
            ((uint32_t)legacy_memory->bytes[offset + 2u] << 16) |
            ((uint32_t)legacy_memory->bytes[offset + 3u] << 24);
        if (legacy_word != fetched) return RE_CPU_E_FETCH_MISMATCH;
    }

    return re_guest_cpu_step(cpu, legacy_memory);
}
