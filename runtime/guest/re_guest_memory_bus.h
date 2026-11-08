#ifndef RE_GUEST_MEMORY_BUS_H
#define RE_GUEST_MEMORY_BUS_H
#include "re_guest_memory.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { const REGUESTMEMORY *memory; } REGUESTMEMBUS;
void re_guest_memory_bus_init(REGUESTMEMBUS *bus, const REGUESTMEMORY *memory);
REGUESTMEMStatus re_guest_memory_fetch8(const REGUESTMEMBUS *bus,uint32_t address,uint8_t *value);
REGUESTMEMStatus re_guest_memory_fetch16(const REGUESTMEMBUS *bus,uint32_t address,uint16_t *value);
REGUESTMEMStatus re_guest_memory_fetch32(const REGUESTMEMBUS *bus,uint32_t address,uint32_t *value);
REGUESTMEMStatus re_guest_memory_read8(const REGUESTMEMBUS *bus,uint32_t address,uint8_t *value);
REGUESTMEMStatus re_guest_memory_read16(const REGUESTMEMBUS *bus,uint32_t address,uint16_t *value);
REGUESTMEMStatus re_guest_memory_read32(const REGUESTMEMBUS *bus,uint32_t address,uint32_t *value);
REGUESTMEMStatus re_guest_memory_write8(REGUESTMEMBUS *bus,uint32_t address,uint8_t value);
REGUESTMEMStatus re_guest_memory_write16(REGUESTMEMBUS *bus,uint32_t address,uint16_t value);
REGUESTMEMStatus re_guest_memory_write32(REGUESTMEMBUS *bus,uint32_t address,uint32_t value);
#ifdef __cplusplus
}
#endif
#endif
