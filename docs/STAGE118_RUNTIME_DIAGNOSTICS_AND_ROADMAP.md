# Stage 118 — Guest Runtime Diagnostics and Roadmap Reset

## Result

Stage 118 adds explicit stop/fault diagnostics to the ARM32 guest runtime coordinator. The runtime now records the guest PC, ARM/Thumb state, stop reason, CPU status, and import-dispatch status when execution cannot continue. Diagnostics are cleared after a subsequent successful instruction or import dispatch.

The test suite now exercises three failure classes:
- Unresolved import placeholder: records the guest PC and `RE_IMPORT_E_UNBOUND`.
- Import dispatch with an invalid/misaligned guest stack: records the trampoline PC and `RE_IMPORT_E_MEMORY`.
- Guest instruction fetch outside mapped memory: records the failing PC and `RE_CPU_E_MEMORY`.

This is host-side runtime behavior validation. It does not establish compatibility with every ARMv7 instruction, the complete XE3U image, or the physical Android device.

## Android ARM64 build status

Stage 117 compiled the guest CPU, AAPCS32 dispatcher, runtime coordinator, and AAPCS32 support into the Android ARM64 shared library. The Android ARM64 workflow completed successfully on commit `b090f90`, including SDK/NDK provisioning, ARM64 liblzma build, APK assembly, ELF verification, artifact collection, and upload. Source Checks also passed.

The Android app's native probe contains a synthetic Thumb instruction and synthetic import-handler smoke path. The CI result proves the probe compiles and links. It does **not** prove the app was installed/launched on a physical device or that the synthetic JNI probe executed there.

## Current architecture boundary

- Host AArch64 process executes a software interpreter for guest ARM32/Thumb instructions.
- Guest registers, CPSR, PC, LR, SP, and guest pointers remain 32-bit values.
- Guest memory is represented by a bounded byte region with a 32-bit guest base.
- External API calls are intercepted through virtual Thumb trampolines and marshalled with AAPCS32.
- DTRZ archive parser/VFS, optional LZMA-alone decoder, and generic file backend exist as separate modules.
- Original executable loading, relocation, guest memory layout, full import registration, graphics/audio/input backends, and actual game boot are not yet integrated.

## Known issues and research tasks

### P0 — Before attempting original game entry
1. **XE3U loader specification:** verify exact header fields, code/data memory extents, alignment, zero-fill/BSS rules, and whether any segments require distinct permissions.
2. **Relocation correctness:** reconstruct all internal relocation types and external Thumb relocation semantics; test against the actual user-supplied image and compare patched pointers with independent calculations.
3. **Guest address-space layout:** establish non-overlapping code, initialized data, BSS, heap, stack, archive buffers, and import trampoline ranges. Preserve 32-bit guest addresses and reject overflow/wraparound.
4. **CPU correctness:** build an instruction coverage inventory from the actual executable and implement missing ARM/Thumb encodings only with encoding-level tests. Audit PC pipeline behavior, flags, shifts, unaligned access, block transfers, interworking, IT blocks, and exception/undefined behavior.
5. **Runtime fault telemetry:** retain stop PC, opcode bytes, register snapshot, CPSR, instruction count, and import symbol when available; ensure diagnostics do not mutate guest state.

### P1 — Runtime services
6. Integrate DTRZ archive parsing, decoder, VFS, and file adapter behind a single owned runtime context. Define lifetime/cleanup order and memory ceilings.
7. Revisit the Stage 97 file API evidence. Item-count return is established, but partial-item, EOF, and error semantics are not fully closed; production `s3eFile*` imports stay disabled until then.
8. Build a complete import inventory from all 227 symbols; classify each as verified, partially understood, unsupported, or intentionally stubbed. No success-returning placeholder for unknown APIs.
9. Reconstruct initialization/shutdown sequencing and required Marmalade services from binary call paths and configuration assets.

### P2 — Platform backends
10. Implement GL/GLES context, texture, buffer, shader, state, and presentation paths with a traceable symbol-to-handler map. Existing two verified GL parameter handlers are not a complete renderer.
11. Investigate audio, input/touch, timing, threading, filesystem paths, and Android lifecycle callbacks.
12. Add device-side diagnostics, frame timing, memory telemetry, and crash-safe logging.

### P3 — Validation and game compatibility
13. Run the native probe on a physical ARM64 device and preserve the exact device/build log.
14. Add differential tests for guest CPU and import behavior against independent emulation/reference results where practical.
15. Reach loader-only boot, then runtime initialization, menu rendering, input, audio, and gameplay as separately evidenced milestones.

## Roadmap

| Stage | Scope | Exit criteria |
|---|---|---|
| 1–100 | Reverse engineering, DTRZ/VFS, file bridge research, toolchain and Android build foundation | Existing reports and host tests; production file imports remain quarantined |
| 101–116 | Android asset staging, pinned toolchain, ARM64 liblzma, CI artifact pipeline | Verified successful Android ARM64 build and artifact upload |
| 117 | Compile guest CPU/runtime/import dispatcher into Android native target; add synthetic probe | Source Checks and Android ARM64 build both green |
| 118 | Runtime fault/stop diagnostics and test coverage | Host runtime tests cover unresolved import, import failure, and CPU fetch failure |
| 119 | Guest memory map and XE3U loader design | Documented address map, overflow-safe region manager, loader fixture tests |
| 120 | XE3U segment loader and relocation engine | Independent relocation tests against the supplied image; no production execution yet |
| 121 | CPU instruction coverage expansion | Evidence-based opcode inventory and regression suite |
| 122 | Runtime context + DTRZ integration | Owned lifecycle, archive open/read tests, Android build |
| 123 | Verified S3E import registration | Per-symbol evidence and ABI/return tests; unknown imports fail closed |
| 124 | Physical-device native probe | Actual device execution log and memory/lifecycle results |
| 125+ | GL/GLES, audio, input, boot sequencing, game initialization | Each subsystem separately demonstrated before claiming playable status |

Stage numbers after 118 are planning targets, not completed work. Dependencies may change after XE3U loader and CPU research.

## Completion status

- Android ARM64 native build: CI verified on Stage 117 commit `b090f90`.
- Source Checks: CI verified on Stage 117 commit `b090f90`.
- Stage 118 runtime diagnostic code: committed; awaiting its own CI run.
- Physical-device execution: not verified.
- Original game boot/gameplay: not achieved.
- Production S3E file imports: disabled pending exact edge-case semantics and image binding verification.
