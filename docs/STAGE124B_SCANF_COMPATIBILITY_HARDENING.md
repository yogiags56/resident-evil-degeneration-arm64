# Stage 124B — Scanf compatibility and parser hardening

## Scope

Stage 124B hardens the Stage 124A bounded scanf-family parser and expands
regression coverage without changing the ARM32 guest ABI contract.

## Changes

- Validate floating conversion length modifiers before assignment suppression,
  so unsupported modifiers cannot be silently accepted by `%*...`.
- Validate parsed integer values against the actual guest destination width:
  `hh` (8-bit), `h` (16-bit), ARM32 default/`l`/`z`/`t` (32-bit),
  and `ll`/`j` (64-bit).
- Reject out-of-range values as matching failures and do not write a truncated
  value into guest memory.
- Extend tests for base-autodetect `%i`, octal `%o`, narrow unsigned stores,
  floating-prefix conversion, suppressed string width, inverted scansets,
  partial matching failure, malformed zero width, and integer overflow.

## Important implementation contract

The scanner remains bounded by the existing input/format and token limits.
Destination writes are still validated through the guest memory bus. Suppressed
conversions consume input but do not consume an AAPCS32 destination argument.
The parser does not delegate guest pointers to host scanf functions.

## Remaining compatibility work

This stage does not claim full ISO C/POSIX scanf conformance. Wide characters,
locale-sensitive decimal parsing, positional arguments, GNU allocation
modifiers, long double, and full floating input-item edge cases remain
deferred. Stream-level behavior for non-seekable sources is also not included
in this parser-hardening stage.

## Validation

The host regression and Android ARM64 build are the release gates. CI status
must be checked against the latest commit before marking the stage complete.
