# Stage 123I — Guest dynamic-loader bridge primitive

## Purpose

Stage 123H proved that `libjirobiodg.so` initializes up to three dynamic-loader handles, searches them through `dlsym`, and closes them during teardown. This stage adds the reusable guest-side bridge primitive needed to implement those calls without exposing 64-bit host pointers to the ARM32 guest.

## Implementation

New files:
- `runtime/guest/re_guest_dl_bridge.h`
- `runtime/guest/re_guest_dl_bridge.c`
- `tests/test_re_guest_dl_bridge.c`

The module provides:
- Callback-driven `dlopen`, `dlsym`, and `dlclose`.
- POSIX-style one-shot `dlerror` retrieval into a caller-supplied guest-memory buffer.
- Up to 8 simultaneous guest handles.
- Best-effort `close_all` shutdown cleanup; failed host closes leave their slots active for retry rather than losing the handle.
- Opaque 32-bit tokens tagged with `0xD1` and a generation counter; stale tokens are rejected after close/reuse.
- Host-native handles are stored only in the host-side bridge structure as `uintptr_t`; they are never written into guest memory.
- The `dlsym` callback must return a 32-bit guest-callable address (for example, a symbol-specific Thumb trampoline). The bridge preserves the returned bit 0 and never returns a raw host function pointer.
- Mapped AAPCS32 handlers for the four functions can be registered transactionally into the existing `REGUESTIMPORTTable` with caller-provided unique binding IDs.
- `re_guest_dl_bridge_build_provider_symbols()` exports the four guest trampoline addresses as `REGUESTSYMBOL` entries tagged with provider SONAME `libdl.so`; this list can be passed directly to `re_guest_elf32_load_linked_image()` as part of its caller-supplied `loaded_providers` catalog.

The module is compiled into the Android ARM64 native target through `app/src/main/cpp/CMakeLists.txt`.

## Validation

Host source-check workflow run `42884464704` passed, including capacity and retryable `close_all` coverage. Its log explicitly reports:

`Guest dlopen/dlsym/dlclose/dlerror bridge tests: PASS`

The synthetic runtime test also resolves the exported `dlsym` entry through `re_guest_symbol_resolve()` using its `libdl.so` provider identity. Host source-check run `42885707892` passed (312 Python tests plus native regressions, including `Guest dlopen/dlsym/dlclose/dlerror bridge tests: PASS`). Android ARM64 build for this provider-export addition is running in run `42885707759`.

The synthetic runtime test exercises:
- `dlopen("libGLESv1_CM.so", 2)` and opaque token creation.
- `dlsym(token, "glTexImage2D")` returning a guest Thumb pointer.
- Missing-symbol behavior and one-shot `dlerror` string delivery.
- `dlclose`, stale-token rejection, and generation change after reopening.
- All eight slots filled, ninth open rejected, failed close-all retained for retry, and successful close-all invalidated all tokens.
- Failed `dlopen` and error retrieval.

Android ARM64 build workflow run `42884437640` passed, including the `close_all` cleanup addition in the ARM64 native target.

## Explicit non-claims

This is a bridge primitive, not yet the production backend for the original provider:
- The callbacks in the test are synthetic; no host `dlopen` or `dlsym` is called.
- The bridge is not yet bound to `libjirobiodg.so`'s actual dynamic relocation symbol indices. Stage 123H traced `JNI_OnLoad` at `0x34056` to the GL initializer at `0x3d3b0`; therefore the real callback/provider registry must be installed before guest `JNI_OnLoad` is invoked.
- No `OGLES`/ `OGLES2` library family is mapped to host GLES/EGL yet.
- `libhgl.so` behavior is unresolved.
- The six `DT_NEEDED` dependencies and 137 relocation-referenced undefined symbols remain to be satisfied.
- No ELF32 provider has been loaded or executed; no game code or GLES call has run.

## Next

1. Add a host provider registry that maps the recovered `OGLES`/`OGLES2` defaults to verified ARM64 GLES/EGL backends and guest Thumb trampolines.
2. Decide whether `libhgl.so` is an optional compatibility provider, an S3E shim, or a required module; fail closed until identified.
3. Bind the four bridge handlers to the exact `dlopen`, `dlsym`, `dlclose`, and `dlerror` ELF import symbols using module-qualified IDs.
4. Build virtual system-provider coverage for the six `DT_NEEDED` libraries and audit all 137 symbol-based relocation references across 136 unique undefined names before attempting ELF32 load.
5. Run a complete read-only ELF32 relocation plan, reserve a 32-bit guest address range with the required `0x8000` alignment, and only then materialize the provider.
