# Stage 123X — Guest libc printf output sink

## Evidence

The original `libjirobiodg.so` relocation manifest lists `printf` as an
undefined, relocation-backed symbol in the direct `libc.so` dependency.
Stage 123W already extracted a bounded AAPCS32 formatter and provided
`snprintf` and `sprintf`; this stage adds the stdout-facing `printf`
entry point without exposing host stdio or a host function pointer to guest
code.

## Implementation

The libc formatted-output provider now binds three exact relocation-backed
symbols: `snprintf`, `sprintf`, and `printf`.

For `printf(format, ...)`, the format pointer is read from guest r0 and
variadic words begin at AAPCS32 core-word index 1 (r1). The shared bounded
formatter reads format and string operands from checked guest memory and
produces output in its fixed 2 KiB native scratch buffer.

The provider forwards the completed byte sequence to a caller-supplied
`REGUESTLIBCFormatOutput` callback. The callback must consume exactly the
requested byte count and return that count; negative or short writes fail
closed. The callback and its user context remain caller-owned and must
outlive the import bindings. No guest `FILE *`, host `FILE *`, or native
callback address is exposed as a guest pointer.

The output callback is optional at provider initialization. A guest call to
`printf` fails closed until the caller installs a valid output sink using
`re_guest_libc_format_provider_set_output`.

## Regression coverage

The formatted-output provider test now binds all three symbols and verifies
that a guest Thumb call to `printf` uses r0 for the format pointer, starts
variadic arguments at r1, formats a guest string and integer, sends the exact
five output bytes to the capture sink, and returns the number of bytes
written. Existing `snprintf` and `sprintf` regressions remain in the same
test.

## Validation boundary

Host regression CI passed in run `43419274585`, including the new guest\n`printf` callback assertion and the existing `snprintf`/`sprintf` regressions.\nAndroid ARM64 CI for the Stage 123X native source is still running. The
provider is compiled into the native target, but production module-loader
installation, a real Android stdout/log destination, original-game execution,
and device boot are not claimed.

## Next

1. Validate host regression and Android ARM64 build.
2. Continue from the original import manifest with guest stdio/file support,
   beginning with a guest-owned FILE abstraction and explicit file backend;
   never pass guest FILE pointers directly to host stdio.
3. Install the provider registry only after callback lifetime and module
   initialization ordering are explicit.
