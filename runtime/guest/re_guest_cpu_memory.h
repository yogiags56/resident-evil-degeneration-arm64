#ifndef RE_GUEST_CPU_MEMORY_H
#define RE_GUEST_CPU_MEMORY_H

#include "re_guest_cpu.h"
#include "re_guest_memory_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CPU data-memory facade. Instruction fetch remains in the CPU bus adapter.
 * Read operations require READ; write operations require WRITE.
 */
typedef struct {
    REGUESTMEMBUS *bus;
} REGUESTCPUMEMORY;

void re_guest_cpu_memory_init(REGUESTCPUMEMORY *cpu_memory,
    REGUESTMEMBUS *memory_bus);

REGUESTCPUStatus re_guest_cpu_mem_read8(const REGUESTCPUMEMORY *cpu_memory,
    uint32_t address, uint8_t *value);
REGUESTCPUStatus re_guest_cpu_mem_read16(const REGUESTCPUMEMORY *cpu_memory,
    uint32_t address, uint16_t *value);
REGUESTCPUStatus re_guest_cpu_mem_read32(const REGUESTCPUMEMORY *cpu_memory,
    uint32_t address, uint32_t *value);

REGUESTCPUStatus re_guest_cpu_mem_write8(const REGUESTCPUMEMORY *cpu_memory,
    uint32_t address, uint8_t value);
REGUESTCPUStatus re_guest_cpu_mem_write16(const REGUESTCPUMEMORY *cpu_memory,
    uint32_t address, uint16_t value);
REGUESTCPUStatus re_guest_cpu_mem_write32(const REGUESTCPUMEMORY *cpu_memory,
    uint32_t address, uint32_t value);

#ifdef __cplusplus
}
#endif
#endif
