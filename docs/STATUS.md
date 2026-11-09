# Current Status

**Current source milestone: Stage 119M research report committed on branch `stage-119m-armv7-alignment`; host CI not yet run for this branch.** Stage 117 integrated the partial ARM32/Thumb guest CPU, AAPCS32 dispatcher, runtime coordinator, and ABI support into the Android ARM64 native target. A prior Android ARM64 workflow passed on commit `b090f90`; that is not a validation of this Stage 119M branch.

Stage 118 adds guest runtime stop diagnostics. Stages 119A–119L progressively introduced the guest memory manager, checked multi-region bus, CPU bus adapters, checked instruction fetch, CPU data-memory facade, isolated A32 transfer helper, and transfer fault regressions. Stage 119M records ARMv7-A/R alignment behavior and explicitly preserves strict alignment until memory attributes and guest SCTLR.A state are modeled.

**Still blocked / unverified:** full ARM/Thumb instruction coverage; XE3U loader and relocation engine; guest address-space map and memory attributes; production file-import activation and exact partial-read/error behavior; complete S3E import table; renderer/audio/input; physical-device execution; original game boot/gameplay. The original APK and game executable are not distributed in the repository.

See [Stage 119M research report](STAGE119M_ARMV7_UNALIGNED_ACCESS_RESEARCH.md) and [Stage 118 report](STAGE118_RUNTIME_DIAGNOSTICS_AND_ROADMAP.md).


## Stage 119 update

Initial guest memory manager committed. The new region table maps bounded 32-bit guest ranges onto caller-owned host buffers, rejects overlaps and 32-bit address-space overflow, and enforces read/write/execute permissions on resolution. Tests cover map/resolve/permission/bounds/overlap/unmap. The module is included in Android CMake and host test runner. It is not yet connected to the CPU interpreter; the current CPU still uses its legacy contiguous `REGUESTMEM` interface. A safe adapter and memory-access migration remain required before XE3U loading.


## Stage 119C update

Stage 119C hardens guest address arithmetic and adds a bounded single-region compatibility view for the legacy CPU memory structure. Tests cover the 4 GiB boundary, SIZE_MAX-sized requests, invalid permissions, region capacity, and adapter permission checks. The adapter is compiled in host tests and Android CMake. It is not a multi-region memory bus and does not yet connect the region manager to CPU execution. See [Stage 119C report](STAGE119C_MEMORY_HARDENING.md).


## Stage 119E–119L updates

Stages 119E–119G added the checked multi-region bus, CPU bus adapter, and checked ARM/Thumb instruction fetch. Stages 119H–119I added an opt-in checked CPU step and regression cases. Stage 119J added the CPU-facing data-memory facade; 119K added an isolated A32 immediate LDR/STR and LDRB/STRB helper; 119L added permission and unmapped-access fault regressions. These remain isolated from the production CPU interpreter; legacy contiguous-memory execution is still active. See the individual Stage 119 reports.


## Stage 119M update

Added `docs/STAGE119M_ARMV7_UNALIGNED_ACCESS_RESEARCH.md`. The ARMv7-A/R architecture table distinguishes byte, halfword, word, exclusive, doubleword, multiple-register, and coprocessor/vector transfers, and shows that SCTLR.A controls unaligned Normal-memory LDR/STR and halfword operations. Exclusive and several multi-register/doubleword families remain alignment-faulting regardless of SCTLR.A. Device and Strongly-ordered memory restrictions and the lack of single-copy atomicity for unaligned accesses are also recorded.

No C implementation or runtime behavior changed. Stage 119J's strict alignment remains the safe default because the current region map does not model memory type and the checked CPU path does not yet model guest SCTLR.A. The next milestone is Stage 119N: add conservative memory attributes and an explicit alignment policy, with tests, before enabling any permissive unaligned access. Stage 119M is a documentation/research completion only; CI validation for its branch is pending.
