#ifndef RE_GUEST_CPU_ARM_SCALAR_H
#define RE_GUEST_CPU_ARM_SCALAR_H

#include "re_guest_cpu.h"
#include "re_guest_cpu_scalar.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Isolated A32 scalar-transfer subset:
 * - condition AL only;
 * - immediate offsets only;
 * - pre-indexed, no writeback;
 * - Rn/Rd cannot be PC;
 * - ordinary LDR/STR byte/word and LDRH/STRH/LDRSB/LDRSH.
 * CPU destination registers are committed only after successful reads.
 */
REGUESTCPUStatus re_guest_cpu_arm_scalar_transfer(
    REGUESTCPU *cpu,
    const REGUESTCPUSCALAR *memory,
    uint32_t instruction
);

#ifdef __cplusplus
}
#endif
#endif
