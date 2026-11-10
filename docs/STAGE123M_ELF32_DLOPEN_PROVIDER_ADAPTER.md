# Stage 123M — ELF32 module exports to guest dlopen providers

## Purpose

Stage 123L introduced an exact-SONAME dlopen provider catalog for
caller-supplied guest exports. Stage 123M connects that catalog to the
existing ELF32 provider collector so already-parsed and already-mapped guest
modules can populate the registry without exposing host pointers.

## Implementation

Added:

- runtime/guest/re_guest_dl_elf32_adapter.h
- runtime/guest/re_guest_dl_elf32_adapter.c
- tests/test_re_guest_dl_elf32_adapter.c

The adapter collects each module independently, preserving SONAME boundaries,
uses the ELF32 collector's load-bias rules (including unrebased SHN_ABS),
preserves Thumb state bits and even-valued data addresses, and builds
per-module export slices in caller-owned storage. It initializes a temporary
dlopen catalog and commits it only after all modules and SONAMEs validate.

The caller owns and must retain the supplied symbol storage and all source
module symbol/name storage for as long as the catalog may be used. The scratch
symbol storage may be modified on failure; the live catalog is not.

## Validation

The host regression verifies two distinct ELF32 modules become two providers,
ordinary symbols are rebased but SHN_ABS is not, Thumb/data pointer values are
preserved, dlsym is scoped to the opened provider, and insufficient storage
or duplicate SONAMEs do not replace the live catalog.

## Limitations

This is an integration adapter, not a production Android dynamic linker. It
does not open Android DSOs, discover system library exports, validate GLES/EGL
ABI compatibility, create ARM32 trampolines, or wire the catalog into the
real guest JNI_OnLoad path. Those remain gates before game boot.
