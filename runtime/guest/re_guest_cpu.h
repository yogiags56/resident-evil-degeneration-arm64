#ifndef RE_GUEST_CPU_H
#define RE_GUEST_CPU_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint32_t r[16], cpsr; uint8_t thumb, halted; uint8_t itstate; uint8_t reserved; uint64_t steps; } REGUESTCPU;
typedef struct { uint8_t *bytes; size_t size; uint32_t guest_base; } REGUESTMEM;
typedef enum { RE_CPU_OK=0, RE_CPU_HALTED=1, RE_CPU_E_ARGUMENT=-1, RE_CPU_E_MEMORY=-2, RE_CPU_E_UNIMPLEMENTED=-3, RE_CPU_E_ALIGNMENT=-4 } REGUESTCPUStatus;
enum { RE_CPSR_N=1u<<31, RE_CPSR_Z=1u<<30, RE_CPSR_C=1u<<29, RE_CPSR_V=1u<<28 };
void re_guest_cpu_reset(REGUESTCPU*, uint32_t entry, int thumb);
REGUESTCPUStatus re_guest_cpu_step(REGUESTCPU*, const REGUESTMEM*);
REGUESTCPUStatus re_guest_cpu_run(REGUESTCPU*, const REGUESTMEM*, uint64_t max_steps, uint64_t *steps_done);
#endif
