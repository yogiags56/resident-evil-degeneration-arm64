# Current Status

**Current source milestone: Stage 119O — checked scalar transfer helper implemented on branch `stage-119o-scalar-transfer`; branch CI pending.** Stage 117 integrated a partial ARM32/Thumb guest CPU, AAPCS32 dispatcher, runtime coordinator, and ABI support into the Android ARM64 native target. A prior Android ARM64 workflow passed on commit `b090f90`; this is not validation of the current branch.

Stage 118 adds guest runtime stop diagnostics. Stages 119A–119L progressively introduced guest memory management, the checked multi-region bus, CPU bus adapters, checked instruction fetch, CPU data-memory facade, isolated A32 transfer helper, and fault regressions. Stage 119M documented ARMv7-A/R alignment rules. Stage 119N added memory attributes and a strict-by-default SCTLR.A policy; final host CI passed on commit `529a7ee`.

**Stage 119O:** added `REGUESTCPUSCALAR`, an isolated byte/halfword/word access helper using explicit little-endian byte assembly, memory-type checks, and the Stage 119N alignment policy. Added tests and Android CMake/host-test registration. The helper is not wired into the production interpreter; the existing memory bus and Stage 119K/L transfer helper remain strict. CI for the current Stage 119O branch is pending.

**Still blocked / unverified:** guest SCTLR system-register emulation and reset-value evidence; exact guest address-space/MMU mapping and page semantics; full ARM/Thumb instruction coverage; XE3U loader and relocation engine; production file-import activation and exact partial-read/error behavior; complete S3E import table; renderer/audio/input; physical-device execution; original game boot/gameplay. The original APK and game executable are not distributed in the repository.

See [Stage 119O report](STAGE119O_CHECKED_SCALAR_TRANSFER.md), [Stage 119N report](STAGE119N_MEMORY_ATTRIBUTES_ALIGNMENT_POLICY.md), [Stage 119M research](STAGE119M_ARMV7_UNALIGNED_ACCESS_RESEARCH.md), and [roadmap](../ROADMAP.md).


## Stage 119 update

Guest memory manager and checked memory bus introduced. The region manager maps bounded guest ranges onto caller-owned host buffers and enforces permissions. It is not yet connected to production CPU execution.


## Stage 119E–119L updates

Stages 119E–119G added the checked multi-region bus, CPU bus adapter, and checked ARM/Thumb instruction fetch. Stages 119H–119I added an opt-in checked CPU step. Stage 119J added the CPU-facing data-memory facade; 119K added an isolated A32 immediate LDR/STR and LDRB/STRB helper; 119L added permission and unmapped-access fault regressions. These remain isolated from the production interpreter.


## Stage 119M update

Added ARMv7-A/R alignment research covering instruction-specific alignment behavior, SCTLR.A, memory-type restrictions, and multi-access fault caveats. Documentation-only stage passed GitHub Actions.


## Stage 119N update

Added UNKNOWN, NORMAL, DEVICE, and STRONGLY_ORDERED guest memory attributes, a source-compatible legacy map API defaulting to UNKNOWN, bounded memory-type queries, and a strict-by-default SCTLR.A policy. Host regression tests passed in GitHub Actions on commit `529a7ee`. The policy remained decision-only.


## Stage 119O update

Added checked scalar data transfers for byte, halfword, and word widths. The helper uses explicit little-endian byte assembly, consults memory attributes and alignment policy, and checks the full access range and permissions before touching memory. Failed stores do not mutate backing bytes and failed reads preserve the output argument. Added tests and Android/host build registration. This helper remains opt-in and is not connected to the production CPU interpreter. Current branch CI is pending.
