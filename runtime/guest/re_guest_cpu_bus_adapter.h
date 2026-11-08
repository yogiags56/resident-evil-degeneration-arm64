#ifndef RE_GUEST_CPU_BUS_ADAPTER_H
#define RE_GUEST_CPU_BUS_ADAPTER_H

#include "re_guest_memory_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Transitional CPU-facing facade. It does not change REGUESTMEM or CPU ABI. */
typedef struct {
    REGUESTMEMBUS *bus;
} REGUESTCPUBUS;

void re_guest_cpu_bus_init(REGUESTCPUBUS *cpu_bus,
    REGUESTMEMBUS *memory_bus);

REGUESTMEMStatus re_guest_cpu_fetch16(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint16_t *value);
/* Fetch one ARM/Thumb instruction. Thumb-2 is fetched as two checked halfwords. */
REGUESTMEMStatus re_guest_cpu_fetch_instruction(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, int thumb_state, uint32_t *instruction, uint8_t *width);
REGUESTMEMStatus re_guest_cpu_fetch32(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint32_t *value);
REGUESTMEMStatus re_guest_cpu_read8(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint8_t *value);
REGUESTMEMStatus re_guest_cpu_read16(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint16_t *value);
REGUESTMEMStatus re_guest_cpu_read32(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint32_t *value);
REGUESTMEMStatus re_guest_cpu_write8(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint8_t value);
REGUESTMEMStatus re_guest_cpu_write16(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint16_t value);
REGUESTMEMStatus re_guest_cpu_write32(const REGUESTCPUBUS *cpu_bus,
    uint32_t address, uint32_t value);

#ifdef __cplusplus
}
#endif
#endif
