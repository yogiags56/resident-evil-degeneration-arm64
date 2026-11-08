#ifndef RE_GUEST_CPU_TRANSFER_H
#define RE_GUEST_CPU_TRANSFER_H

#include "re_guest_cpu.h"
#include "re_guest_cpu_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Execute the conservative A32 immediate single-data-transfer subset:
 * condition AL, immediate offset, pre-index, no writeback, Rn/Rd != PC.
 * Returns RE_CPU_E_UNIMPLEMENTED for encodings outside this documented subset.
 * CPU registers are committed only after a successful memory operation.
 */
REGUESTCPUStatus re_guest_cpu_arm_transfer_imm(
    REGUESTCPU *cpu, const REGUESTCPUMEMORY *memory, uint32_t instruction);

#ifdef __cplusplus
}
#endif
#endif
