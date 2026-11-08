# Stage 95 — Guest File API Bridge Readiness Audit

## Outcome

Stage 95 consolidates the currently recoverable `s3eFile*` ABI evidence and adds typed, host-testable argument-capture helpers for `s3eFileOpen`, `s3eFileGetChar`, and `s3eFileClose`. The helpers are evidence/audit utilities only: they do not invoke a backend, dereference guest pointers, mutate guest registers, or register import handlers.

**Guest file import activation remains disabled.** In particular, the `s3eFileRead` return value and short-read/error semantics remain unproven.

## Evidence matrix

| API | Guest evidence | Supported conclusion | Still unknown / gate |
|---|---|---|---|
| `s3eFileOpen` | Stage 81: callsites `0x139FE4`, `0x13A16E`; veneer `0xA5C` resolves to slot `0x13BA50`. | `r0` is a path pointer, `r1` is a mode pointer; observed mode is `rb`. Return is tested for null and retained as a handle-like value. | Exact SDK prototype/return type, all path namespaces, handle ownership, and all failure cleanup paths. |
| `s3eFileRead` | Stage 81/84: veneer `0x30C` resolves to slot `0x13B7E0`; calls include `0x13A00E`, `0x13A04C`, `0x13A05E`, `0x13A06A`, `0x13A07A`. | AAPCS32 words are `(destination, item_size, item_count, handle)` in `r0-r3`. | Return units, EOF, partial-item/short-read, and error behavior. Caller return is not consumed in the examined sites. |
| `s3eFileGetChar` | Stage 81/84: veneer `0xAB0` resolves to slot `0x13BA6C`; helper `0x139F8C` tests `-1` as EOF/error. | One handle in `r0`; integer-like result; `-1` is the sentinel expected by the inspected helper. | Exact SDK declaration, other negative/error values, and behavior after close. |
| `s3eFileClose` | Stage 81: callsite `0x13A348`; veneer `0xC0` resolves to slot `0x13B71C`. | One handle in `r0`; caller clears its stored handle after the call. | Exact return type and close-error semantics; complete alias/double-close behavior. |

The evidence above is carried forward from prior reports. The Stage 94 source ZIP does not contain the original APK or decompressed `biohazard.s3e` image, so Stage 95 does not claim a fresh disassembly or expanded original-binary xref scan.

## New source

- `app/src/main/cpp/re_s3e_file_api_audit.h/.c`
  - Captures the proven argument words for `Open`, `GetChar`, and `Close`.
  - Rejects null path/mode/handle arguments at the capture boundary.
  - Exposes an explicit `re_s3e_file_api_guest_imports_authorized()` gate that returns false.
- `tests/test_re_s3e_file_api_audit.c`
  - Exercises successful argument capture, null guards, and the disabled-import gate.

The existing Stage 89 `re_s3e_file_read_contract` remains unchanged and retains the independent `s3eFileRead` return-authorization gate.

## Bridge-readiness findings

1. **AAPCS32 register capture is structurally available.** The import dispatcher snapshots `r0-r3` into `S3EAAPCS32Frame` and routes to a handler only when an explicit symbol binding exists.
2. **The generic adapter handle is a 32-bit opaque token, not a host pointer.** It encodes a slot and generation, rejects stale handles, and therefore can be returned as a guest-visible opaque handle only after import semantics and lifecycle behavior are validated.
3. **The adapter is byte-oriented.** `re_file_adapter_read_bytes()` reports transferred bytes through a host output parameter. This is not evidence that the original `s3eFileRead` returns bytes, items, or status.
4. **`GetChar` is closer to bridge-ready, but not yet authorized.** The guest's `-1` check and backend's `0..255/-1` behavior align for EOF. Error mapping, invalid handles, and guest-visible return propagation still need explicit policy and original-runtime confirmation.
5. **`Open` is not fully bridge-ready.** The guest's observed `rb` mode and adapter's read-only mode agree, but path mapping, exact failure convention, and ownership/error cleanup still need closure.
6. **`Close` is not fully bridge-ready.** The adapter invalidates the handle and releases the backend stream, but guest-visible return behavior and all lifecycle edge cases are not proven.

## Validation

- Stage 95 file API argument audit: 14 assertions passed.
- Existing Stage 89 `s3eFileRead` contract tests passed; return authorization remains false.
- Full host test suite and strict host mock compilation passed after integration.

These are host-side contract tests. They do not prove the original Marmalade SDK implementation, run the guest imports, or boot the game.

## Activation decision

No `s3eFileOpen`, `s3eFileRead`, `s3eFileGetChar`, or `s3eFileClose` guest import is registered. Do not bypass the explicit disabled gates. Do not infer a guest `r0` value from the host adapter's `REFileStatus`, byte count, or EOF value.

## Next stage — Stage 96

- Recover an authoritative Marmalade SDK API reference matching the game's SDK generation, or obtain a controlled observation from the original 32-bit runtime.
- If original runtime execution is available, record `r0` before/after each call for full read, partial read, EOF, missing file, invalid handle, and close.
- Re-audit all `s3eFileRead` callsites with the original decompressed guest image and trace whether any return value is consumed outside the already examined paths.
- Define a guest-visible handle and error mapping only after evidence is sufficient.
- Add import-dispatch integration tests using synthetic guest memory and explicit bindings; keep production symbol-slot patching disabled until every handler contract is proven.

## Not achieved

- No authoritative SDK reference for this exact game build has been recovered.
- No new original-binary disassembly was possible from the Stage 94 source-only archive.
- No guest file import is activated.
- No Android NDK/device runtime validation or original game boot/gameplay has occurred.
