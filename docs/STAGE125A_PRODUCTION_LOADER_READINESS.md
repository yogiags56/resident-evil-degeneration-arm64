# Stage 125A — production module-loader readiness audit

## Executive result

The repository contains the major bounded ELF32 loader building blocks, but it does not yet contain a production Android module-loading path. Stage 125A therefore begins with an integration-readiness audit rather than claiming a production loader is complete.

## Existing components verified

- `re_guest_elf32_pipeline`: parses candidate ELF32 modules, PT_DYNAMIC, DT_NEEDED/DT_SONAME, dynamic symbols and relocations; resolves the root-reachable dependency graph; prepares relocations against a provider catalog; and submits a dependency-first transactional load batch.
- `re_guest_elf32_module_graph`: bounded dependency traversal, duplicate-name/cycle/unresolved-dependency checks and dependency-first ordering.
- `re_guest_elf32_init` / `re_guest_elf32_init_exec`: constructor sequence planning and bounded AArch32 constructor execution with a return trampoline.
- `re_guest_elf32_boot`: validates root entry and stack and initializes the guest runtime, but does not run the game entry point.
- `re_guest_dl_elf32_adapter`, `re_guest_dl_provider_catalog`, and `re_guest_dl_provider_bridge`: caller-supplied ELF32 export catalog and exact-SONAME guest libdl bridge.
- Android CMake already compiles these runtime components.

## Production blockers

1. **No asset/module inventory.** The repository's `app/src/main/assets` contains only `.gitkeep`; there is no XE3U executable or guest DSO payload to discover. The current Android AssetManager backend opens a known path but does not enumerate and materialize a module inventory for the ELF32 pipeline.
2. **No end-to-end runtime coordinator.** The Android JNI probe exercises isolated parser/runtime/provider smoke tests. It does not assemble the full sequence of asset acquisition, module inventory, ELF pipeline, import binding, constructors, root runtime setup, and bounded execution.
3. **Import/provider policy is incomplete.** Exact-SONAME dlopen catalog wiring exists as reusable components, but no production guest runtime lifecycle owns the catalog, symbol storage, handles, import table, and teardown together. Android/Bionic/GLES/EGL ABI coverage and guest-callable ARM32 trampolines remain unverified.
4. **ELF compatibility boundaries remain.** The unified pipeline documentation records unsupported or unverified GNU symbol version/hash-only bounds, TLS, IFUNC, COPY relocations, lazy PLT binding and Bionic namespace semantics.
5. **Real boot evidence is absent.** No original game executable/assets have been loaded or executed; the current root boot helper only initializes the runtime state.

## Stage 125A acceptance gates

- Define an explicit, versioned guest-module manifest format with a root executable identity, exact SONAMEs, asset paths, expected ELF class/machine and optional digest.
- Implement bounded Android asset reads into caller-owned module buffers; reject path traversal, duplicate SONAMEs, size overflow and manifest/ELF identity mismatch.
- Add host tests for manifest parsing, duplicate/missing modules, malformed paths, capacity limits and digest mismatch.
- Add Android instrumentation/native smoke coverage that loads a synthetic manifest and verifies all module bytes and identities before invoking the ELF pipeline.
- Build a single lifecycle coordinator only after module ownership, import/provider lifetime and failure rollback contracts are explicit.
- Keep game entry execution disabled until dependency, relocation, import and constructor gates all pass.

## Validation snapshot

Stage 124N's host test workflow passed on commit `387dffea0baaa01def4a15b16bc3c8ce61c5afc3`. Its Android ARM64 workflow was not triggered by that test/documentation-only change because the workflow's path filters cover `app/**`, `runtime/**`, `tools/**`, `config/**`, `gradle/**`, and build configuration, but not `tests/**` or `docs/**`. The preceding Stage 124M Android ARM64 build passed. No Stage 125A implementation or game boot is claimed by this audit.

## Next implementation slice

Stage 125A.1: define and test the guest-module manifest contract. Do not guess filenames, SONAMEs, ABI requirements, or asset contents; populate the manifest only from verified game/package evidence.
