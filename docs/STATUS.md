# Current Status

**Current source milestone: Stage 119N implementation committed on branch `stage-119n-memory-attributes`; branch CI pending.** Stage 117 integrated the partial ARM32/Thumb guest CPU, AAPCS32 dispatcher, runtime coordinator, and ABI support into the Android ARM64 native target. A prior Android ARM64 workflow passed on commit `b090f90`; that is not validation of the current Stage 119N branch.

Stage 118 adds guest runtime stop diagnostics. Stages 119A–119L progressively introduced the guest memory manager, checked multi-region bus, CPU bus adapters, checked instruction fetch, CPU data-memory facade, isolated A32 transfer helper, and transfer fault regressions. Stage 119M recorded ARMv7-A/R alignment behavior and preserved strict alignment. Stage 119M Source checks passed on GitHub Actions run 152.

**Still blocked / unverified:** full ARM/Thumb instruction coverage; XE3U loader and relocation engine; complete guest address-space map and MMU/page semantics; production file-import activation and exact partial-read/error behavior; complete S3E import table; renderer/audio/input; physical-device execution; original game boot/gameplay. The original APK and game executable are not distributed in the repository.

See [Stage 119N report](STAGE119N_MEMORY_ATTRIBUTES_ALIGNMENT_POLICY.md), [Stage 119M research report](STAGE119M_ARMV7_UNALIGNED_ACCESS_RESEARCH.md), and [project roadmap](../ROADMAP.md).


## Stage 119 update

Initial guest memory manager committed. The region table maps bounded 32-bit guest ranges onto caller-owned host buffers, rejects overlaps and 32-bit address-space overflow, and enforces read/write/execute permissions. It is not yet connected to the production CPU interpreter.


## Stage 119C update

Stage 119C hardens guest address arithmetic and adds a bounded single-region compatibility view for the legacy CPU memory structure. It is not a multi-region memory bus and does not connect the region manager to CPU execution.


## Stage 119E–119L updates

Stages 119E–119G added the checked multi-region bus, CPU bus adapter, and checked ARM/Thumb instruction fetch. Stages 119H–119I added an opt-in checked CPU step and regression cases. Stage 119J added the CPU-facing data-memory facade; 119K added an isolated A32 immediate LDR/STR and LDRB/STRB helper; 119L added permission and unmapped-access fault regressions. These remain isolated from the production CPU interpreter; legacy contiguous-memory execution is still active.


## Stage 119M update

Added ARMv7-A/R alignment research. The report distinguishes byte, halfword, word, exclusive, doubleword, multiple-register, and coprocessor/vector transfers; describes SCTLR.A and memory-type restrictions; and records non-atomic / multi-access fault caveats. This documentation-only stage passed GitHub Actions Source checks (run 152).


## Stage 119N update

Added explicit guest memory attributes: UNKNOWN, NORMAL, DEVICE, and STRONGLY_ORDERED. Existing `re_guest_memory_map` remains source-compatible and maps regions as UNKNOWN; `re_guest_memory_map_ex` accepts an explicit type. Added bounded `re_guest_memory_query_type`.

Added `REGUESTALIGNMENTPOLICY` and `re_guest_alignment_check`. The default policy is strict (SCTLR.A=1). When explicitly configured with SCTLR.A clear, only unaligned scalar halfword/word accesses wholly inside NORMAL memory are eligible. UNKNOWN, DEVICE, STRONGLY_ORDERED, exclusive, doubleword, multiple-register, SWP, and coprocessor unaligned accesses remain rejected. Naturally aligned accesses, including aligned exclusive/doubleword forms, are accepted.

The policy is decision-only: the checked memory bus still rejects unaligned 16/32-bit accesses, and no production instruction behavior changed. Host regression test and Android CMake registration are added. GitHub Actions validation for this branch is pending.
