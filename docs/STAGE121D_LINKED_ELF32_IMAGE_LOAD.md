# Stage 121D — Caller-driven linked ELF32 image load

## Goal
Provide one bounded coordinator that connects the ELF parser, PT_DYNAMIC decoder, dynamic symbol/relocation inventory, caller-supplied provider resolution, relocation planning, and transactional image loader.

## Implemented
- Added a single-image coordinator that parses the ELF image and dynamic section before touching guest memory.
- Decodes SysV-hash-bounded dynamic symbols and REL/RELA/PLT relocation records into caller-provided scratch arrays.
- Resolves undefined symbols against caller-supplied already-loaded provider definitions.
- Prepares relocation targets with load bias and passes the result to the existing transactional ELF32 image loader.
- Uses caller-owned scratch buffers and segment buffers; no hidden allocation is performed.
- Preserves the existing loader's rollback behavior and final segment permission checks.
- Adds a synthetic end-to-end fixture with RX and RW segments, a dynamic undefined import, a REL ABS32 relocation, and a BSS tail. The test also verifies that an unresolved strong import fails before any mapping is created.

## Deliberate limits
This is a caller-driven single-image linked-load path, not an Android/Bionic dynamic linker. The caller must already have selected and loaded providers and supply their final guest virtual symbol addresses. DT_NEEDED search/loading, namespace/scope semantics, symbol interposition, GNU symbol versions, TLS/COPY/IFUNC, constructors, runtime import trampolines, CPU startup, and game execution remain outside this stage.

## Validation
Host tests and Android ARM64 build/ELF verification are triggered for this stage. The stage remains pending until both pipelines pass.
