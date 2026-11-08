# Current Status

**Current source milestone: Stage 118 implementation committed; CI pending.** Stage 117 integrated the partial ARM32/Thumb guest CPU, AAPCS32 dispatcher, runtime coordinator, and ABI support into the Android ARM64 native target. The Android ARM64 workflow passed on commit `b090f90`: SDK/NDK provisioning, ARM64 liblzma compilation, APK assembly, ELF verification, artifact collection, and upload. Source Checks passed on the same commit.

Stage 118 adds guest runtime stop diagnostics: fault PC, ARM/Thumb state, CPU status, import status, and stop reason. Tests cover unresolved imports, import-dispatch failure due to invalid guest stack alignment, and guest instruction fetch outside mapped memory. The Stage 118 report documents the forward roadmap and outstanding research. Its CI validation is pending.

**Still blocked / unverified:** full ARM/Thumb instruction coverage; XE3U loader and relocation engine; guest address-space map; production file-import activation and exact partial-read/error behavior; complete S3E import table; renderer/audio/input; physical-device execution; original game boot/gameplay. The original APK and game executable are not distributed in the repository.

See [Stage 118 report](STAGE118_RUNTIME_DIAGNOSTICS_AND_ROADMAP.md).


## Stage 119 update

Initial guest memory manager committed. The new region table maps bounded 32-bit guest ranges onto caller-owned host buffers, rejects overlaps and 32-bit address-space overflow, and enforces read/write/execute permissions on resolution. Tests cover map/resolve/permission/bounds/overlap/unmap. The module is included in Android CMake and host test runner. It is not yet connected to the CPU interpreter; the current CPU still uses its legacy contiguous `REGUESTMEM` interface. A safe adapter and memory-access migration remain required before XE3U loading.


## Stage 119C update

Stage 119C hardens guest address arithmetic and adds a bounded single-region compatibility view for the legacy CPU memory structure. Tests cover the 4 GiB boundary, SIZE_MAX-sized requests, invalid permissions, region capacity, and adapter permission checks. The adapter is compiled in host tests and Android CMake. **It is not a multi-region memory bus and does not yet connect the region manager to CPU execution.** Full CPU access migration and XE3U loader fixtures remain Stage 119 exit criteria. See [Stage 119C report](STAGE119C_MEMORY_HARDENING.md).


## Stage 119E update

Added the checked multi-region guest memory bus in `runtime/guest/re_guest_memory_bus.h/.c`. It exposes separate instruction-fetch (EXEC), data-read (READ), and data-write (WRITE) accessors for 8/16/32-bit little-endian values, delegates complete-range and permission validation to the region manager, clears read outputs on failure, and rejects misaligned 16/32-bit accesses. A synthetic test maps executable code, read-only data, and writable data with unmapped gaps and boundary cases. The bus is compiled into Android CMake and the host test runner. **The CPU interpreter is not yet migrated to this bus**; its legacy contiguous memory interface remains active. See [Stage 119E report](STAGE119E_CHECKED_MEMORY_BUS.md).


## Stage 119F update

Added an ABI-preserving CPU-facing adapter over the checked memory bus in `runtime/guest/re_guest_cpu_bus_adapter.h/.c`. The adapter has distinct EXEC fetch, READ data, and WRITE data calls and is covered by a host regression test. It is compiled into Android CMake and registered in the host test runner. This does **not** change the CPU interpreter: `re_guest_cpu_step` still uses the legacy contiguous `REGUESTMEM` path. Stage 119F is therefore an adapter milestone, not completion of CPU memory migration. See [Stage 119F report](STAGE119F_CPU_BUS_ADAPTER.md).


## Stage 119G update

Extended the CPU bus adapter with a checked instruction-fetch helper. ARM fetch uses one 32-bit EXEC access; Thumb fetch uses a 16-bit EXEC access and independently checks the second halfword for Thumb-2 32-bit prefixes. Tests cover permission denial including a Thumb-2 instruction split across adjacent regions with different permissions. The helper is registered in the host test runner. The production interpreter is **not yet migrated** and continues to fetch from legacy contiguous memory. See [Stage 119G report](STAGE119G_CHECKED_INSTRUCTION_FETCH.md).


## Stage 119H update

Added opt-in `re_guest_cpu_step_checked`. It fetches ARM/Thumb instructions through the checked EXEC memory bus, validates Thumb-2 second-halfword access, verifies instruction bytes match the legacy contiguous decoder view, then invokes the existing CPU step. Permission and mismatch failures return before CPU state advances. Added and registered `tests/test_re_guest_cpu_checked_step.c`. The legacy CPU entry point and public memory/CPU structure layouts remain unchanged. This is a transitional dual-view gate: instruction/data accesses inside the decoder still use contiguous legacy memory. See [Stage 119H report](STAGE119H_CHECKED_CPU_STEP.md).


## Stage 119I update

Expanded `tests/test_re_guest_cpu_checked_step.c` with ARM MOV, Thumb-2 MOVW, IT EQ true/false execution, and Thumb-2 second-halfword EXEC-permission denial. Existing Thumb-1, fetch parity, and permission-denial checks remain. The test remains opt-in and does not switch production runtime to the checked path. See [Stage 119I report](STAGE119I_CHECKED_CPU_REGRESSION.md).


## Stage 119J update

Added CPU-facing data-memory facade `re_guest_cpu_memory.h/.c` for checked byte/halfword/word reads and writes through the guest memory bus, with CPU error-status mapping. Added `tests/test_re_guest_cpu_memory.c`, registered it in the host test runner, and added the implementation to Android CMake. This does **not** migrate interpreter load/store opcodes yet; the legacy decoder still uses contiguous memory. Strict bus alignment behavior remains provisional pending ARMv7 alignment research. See [Stage 119J report](STAGE119J_CPU_DATA_MEMORY_FACADE.md).
