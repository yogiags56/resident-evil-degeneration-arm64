# Stage 123L — Guest DSO provider catalog for dlopen/dlsym

## Purpose

Stage 123I supplied guest-callable `dlopen`/`dlsym`/`dlclose`/`dlerror`
handlers, and Stage 123K mapped those handlers from the actual ELF32 dynamic
symbol indices. Stage 123L adds the deterministic callback adapter that makes
those handlers usable with caller-supplied, already verified guest exports.

## Implementation

Added:

- `runtime/guest/re_guest_dl_provider_catalog.h`
- `runtime/guest/re_guest_dl_provider_catalog.c`
- `tests/test_re_guest_dl_provider_catalog.c`

The catalog:

- accepts at most 16 caller-supplied provider SONAMEs;
- validates unique basename-only SONAMEs and bounded, externally visible
  GLOBAL/WEAK symbol inventories;
- accepts only exact SONAME matches (no directory search, path traversal,
  host `dlopen`, or ambient process-global lookup);
- returns a host-private one-based catalog index as the bridge's opaque
  `uintptr_t` handle, never as a guest value;
- tracks multiple opens and closes without invalidating other open references;
- resolves a symbol only inside the provider associated with that handle;
- returns only the supplied 32-bit guest symbol value, preserving Thumb
  state bits and data addresses without converting or exposing host pointers;
- delegates strong/weak and ambiguity behavior to the existing bounded
  `re_guest_symbol_resolve()` implementation;
- initializes transactionally and retains the prior catalog on invalid input.

The adapter is compiled into the Android ARM64 native target and has a
standalone host regression.

## Validation and limitations

The regression covers exact SONAME matching, path rejection, missing
providers/symbols, per-provider symbol scope, repeated opens, reference-counted
close, stale native handles, preservation of both Thumb code pointers and
even-valued data addresses, and transactional initialization failure.

The adapter is intentionally caller-driven. It does **not** load Android DSOs,
call host GLES/EGL functions, synthesize ARM32 trampolines, or decide which
system exports are ABI-compatible. A catalog entry is useful only when its
symbol values already identify valid guest-callable addresses. Versioned
symbol lookup and concurrent catalog mutation are not implemented; callers
must keep source arrays alive and serialize catalog access.

CI must pass before this stage is treated as validated. This stage does not
claim that the original game boots or is playable.
