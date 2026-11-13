# Stage 123T — ARM EABI compiler-helper provider

## Evidence and scope

The Stage 123O relocation-backed import manifest contains 21 unique
`__aeabi_*` names. This stage provides guest ARM32 trampolines for exactly
those names. The implementation is based on Arm's Run-time ABI for the Arm
Architecture (RTABI32), with the base-standard/softfp convention: floating
arguments and results travel through core registers rather than VFP argument
registers.

This is an ABI-family provider, not a claim that the original DSO has a direct
`DT_NEEDED` entry for `libgcc.so` or `libcompiler_rt.so`. Export records are
unversioned and carry no inferred provider SONAME.

## Implemented relocation-backed helpers

- Double arithmetic: `__aeabi_dadd`, `__aeabi_dsub`, `__aeabi_dmul`,
  `__aeabi_ddiv`.
- Double comparisons: `__aeabi_dcmpeq`, `__aeabi_dcmplt`.
- Float arithmetic: `__aeabi_fadd`, `__aeabi_fsub`, `__aeabi_fmul`.
- Floating conversions: `__aeabi_d2f`, `__aeabi_d2iz`, `__aeabi_f2iz`.
- Integer conversions: `__aeabi_i2d`, `__aeabi_ui2d`, `__aeabi_i2f`.
- 32-bit division: `__aeabi_idiv`, `__aeabi_uidiv`,
  `__aeabi_idivmod`, `__aeabi_uidivmod`.
- 64-bit integer operations: `__aeabi_lmul`, `__aeabi_ldivmod`.

The exact undefined ELF32 symbol indices are validated before binding. The
provider binds all 21 or none, and returns guest Thumb trampoline addresses.
It does not expose host function pointers.

## Register and arithmetic contract

- 64-bit arguments use even AAPCS32 core-register word positions. For two
  64-bit operands, the low/high words occupy r0/r1 and r2/r3.
- A double or 64-bit integer result is returned in r0/r1.
- 32-bit `idivmod` and `uidivmod` return quotient in r0 and remainder in r1.
- RTABI32 `__aeabi_ldivmod` returns the 64-bit quotient in r0/r1 and the
  64-bit remainder in r2/r3. The shared import dispatcher was extended to
  preserve four-register return values for this and future ABI helpers.
- The floating helpers operate on IEEE single/double values using host C
  arithmetic, with the host process's normal round-to-nearest environment.
  Comparisons return 1 or 0; unordered NaN comparisons therefore return 0 for
  the two implemented predicates.
- Integer division truncates toward zero. The chosen divide-by-zero policy is
  quotient zero and, for divmod, remainder equal to the numerator. RTABI32
  explicitly permits a fixed `__aeabi_idiv0`/`__aeabi_ldiv0` result and
  allows divmod remainder to be zero or the original numerator.
- Signed minimum divided by -1 returns the minimum value with zero remainder,
  avoiding C signed-overflow undefined behavior.
- Float-to-int conversions truncate toward zero. NaN and out-of-range inputs
  use a deterministic saturating policy to avoid undefined C casts; exact
  original-runtime behavior for invalid conversion operands remains unverified.

## Implementation

Added:

- `runtime/guest/re_guest_eabi_provider.h`
- `runtime/guest/re_guest_eabi_provider.c`
- `tests/test_re_guest_eabi_provider.c`

Updated the shared import result and dispatch path to carry r0-r3. The provider
is registered in the Android ARM64 CMake target and the strict host regression
runner.

## Deliberate limitations

- Only the 21 relocation-backed names from the original manifest are provided.
  This does not implement the complete RTABI32 helper library.
- The status-returning CPSR comparison helpers, additional float/double
  predicates, 64-bit shifts/comparisons, memory helper variants, and other
  compiler helpers remain unsupported unless separately evidenced.
- The two comparison helpers here return their specified Boolean values in
  r0; they do not emulate CPSR flags.
- The provider is not yet installed into the real ELF32 module-load lifecycle.
- No original game boot, constructor execution, or physical-device runtime
  behavior is claimed.

## Validation

The host regression binds all 21 exact symbols and exercises double/float
arithmetic and comparisons, integer conversions, signed/unsigned division,
32-bit divmod, 64-bit multiplication, and 64-bit divmod's r0-r3 return shape.
It also checks the chosen divide-by-zero behavior.

Host CI passed in run `43415885622`: 312 Python tests, native C mock-header
syntax checks, and all compiled host regressions, including
`Guest ARM EABI provider tests: PASS`. Android ARM64 build and ELF
verification passed in run `43415890966`, including the app build, ELF
verification, and build-artifact upload. These are compile/regression results;
no original-game execution or device-side ABI validation is claimed.

Reference: Arm, [Run-time ABI for the Arm Architecture (RTABI32)](https://github.com/ARM-software/abi-aa/blob/main/rtabi32/rtabi32.rst),
especially sections 5.1, 5.2, and 5.3.1–5.3.2.

## Next

1. Complete Android ARM64 compilation and ELF verification with the provider
   registered in CMake.
2. Verify the original module's exact ABI attributes and helper call sites
   before enabling this provider in the production loader.
3. Continue with the C/Bionic import family, starting from relocation-backed
   symbols and separating memory, file, process, threading, and socket APIs.
