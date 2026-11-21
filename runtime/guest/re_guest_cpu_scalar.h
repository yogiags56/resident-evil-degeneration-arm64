#ifndef RE_GUEST_CPU_SCALAR_H
#define RE_GUEST_CPU_SCALAR_H

#include "re_guest_alignment.h"
#include "re_guest_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opt-in checked scalar data access. Instruction fetch and production CPU
 * decoding are deliberately outside this module. */
typedef struct {
 const REGUESTMEMORY *memory;
 const REGUESTALIGNMENTPOLICY *alignment;
} REGUESTCPUSCALAR;

void re_guest_cpu_scalar_init(REGUESTCPUSCALAR *scalar,
 const REGUESTMEMORY *memory,const REGUESTALIGNMENTPOLICY *alignment);

REGUESTCPUStatus re_guest_cpu_scalar_read8(const REGUESTCPUSCALAR *scalar,
 uint32_t address,uint8_t *value);
REGUESTCPUStatus re_guest_cpu_scalar_read16(const REGUESTCPUSCALAR *scalar,
 uint32_t address,uint16_t *value);
REGUESTCPUStatus re_guest_cpu_scalar_read32(const REGUESTCPUSCALAR *scalar,
 uint32_t address,uint32_t *value);
REGUESTCPUStatus re_guest_cpu_scalar_write8(const REGUESTCPUSCALAR *scalar,
 uint32_t address,uint8_t value);
REGUESTCPUStatus re_guest_cpu_scalar_write16(const REGUESTCPUSCALAR *scalar,
 uint32_t address,uint16_t value);
REGUESTCPUStatus re_guest_cpu_scalar_write32(const REGUESTCPUSCALAR *scalar,
 uint32_t address,uint32_t value);

#ifdef __cplusplus
}
#endif
#endif
