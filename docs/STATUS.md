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
