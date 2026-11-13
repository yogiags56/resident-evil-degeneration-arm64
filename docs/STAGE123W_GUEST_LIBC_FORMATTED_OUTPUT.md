# Stage 123W — Guest libc formatted output

## Scope

Adds bounded guest-side adapters for the exact relocation-backed `libc.so`
imports `snprintf` and `sprintf`. The provider binds by exact ELF32 symbol
index and exports guest-callable Thumb trampolines tagged `libc.so`.

The formatter consumes variadic arguments using AAPCS32 core-register and
guest-stack word order, including even-word alignment for 64-bit values. It
reads guest format strings and guest `%s` operands through the checked guest
memory bus. It formats into a fixed 2 KiB host scratch buffer, then copies
only the permitted result range back to guest memory. `snprintf` writes at
most size-1 bytes plus NUL and returns the untruncated output length.
`sprintf` writes the full bounded result plus NUL, subject to a validated
guest destination range.

Supported conversions and flags are deliberately bounded. Positional
arguments, `%n`, unknown conversions, unsupported length modifiers, invalid
guest pointers, unterminated strings, and output overflow fail closed.
This is not a complete Bionic printf implementation; ABI-specific edge
behavior still requires comparison with the original Android runtime.

## Regression coverage

The native guest-runtime test covers exact symbol exports, register/stack
variadic access, output truncation, zero-size `snprintf`, `sprintf`,
64-bit argument alignment, and dynamic width. It also verifies deterministic
missing/duplicate-symbol diagnostics and that failed binding leaves the
import table and export array unchanged.

## Integration boundary

The source is included in the Android ARM64 native CMake target and the
compiled host regression runner. The provider is not yet installed into the
production ELF32 module-loader lifecycle. No original-game execution,
physical-device validation, or playable build is claimed by this stage.

## Validation

GitHub Actions validation for the current Stage 123W head is pending. Record
host and Android ARM64 results here only after their workflows complete.
