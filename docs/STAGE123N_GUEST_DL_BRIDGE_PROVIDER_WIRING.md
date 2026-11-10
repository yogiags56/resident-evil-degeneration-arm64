# Stage 123N — Guest dlopen bridge/provider catalog wiring

## Purpose

Stage 123I implemented the guest-callable dlopen/dlsym/dlclose/dlerror
bridge. Stage 123L added an exact-SONAME DSO provider catalog, and Stage 123M
connected that catalog to exports from already-parsed/mapped ELF32 modules.
Stage 123N installs the catalog callbacks directly into the guest bridge.

## Implementation

Added:

- runtime/guest/re_guest_dl_provider_bridge.h
- runtime/guest/re_guest_dl_provider_bridge.c
- tests/test_re_guest_dl_provider_bridge.c

The helper validates a non-empty bounded provider catalog and calls the
existing bridge initializer with the catalog's exact-SONAME open, provider-
scoped symbol lookup, and reference-counted close callbacks. It does not
create host-to-guest trampolines or load Android libraries.

## Validation

The host regression exercises the real guest bridge API against the real
provider catalog. It checks opaque guest tokens, preservation of Thumb code
and even data addresses, provider reference counts across duplicate opens,
provider closure, stale guest-token rejection, and missing-provider errors.

## Lifecycle contract

The catalog object must remain at a stable address and its backing symbol
storage must remain alive while the bridge uses it. Do not replace or mutate
the catalog while guest handles are open or guest calls are in flight. Shut
down with the bridge close-all operation before destroying the catalog.

## Limitations

This connects the internal guest libdl plumbing, not Android's system linker.
Real Android GLES/EGL exports still require ABI-verified ARM32 callable
trampolines. The actual libjirobiodg.so JNI_OnLoad path is not yet wired to this
helper, and no original-game boot or playability is claimed.
