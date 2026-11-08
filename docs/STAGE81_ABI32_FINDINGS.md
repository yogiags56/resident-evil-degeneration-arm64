# Stage 81 — File API AAPCS32 ABI Validation

## Scope and method
This stage rechecks the original guest disassembly rather than trusting import names alone.
The guest image is ARM32/Thumb and uses the AAPCS32 core-register argument convention:
the first four 32-bit argument words enter in r0-r3. The three-instruction import veneers
were resolved to external import slots using their PC-relative arithmetic.

## 1. `s3eFileOpen`

Observed at `0x139FE4` and again at `0x13A16E`.
At `0x139FE4`, the caller places the input path pointer in r0 and an address of the
NUL-terminated mode string `rb` at `0x13A140` in r1, then calls veneer `0xA5C`.
The veneer resolves to import slot `0x13BA50` (`s3eFileOpen`).
The returned r0 is tested against zero and saved as a file-handle-like value.

Evidence-supported contract:
- argument 0: path/string pointer;
- argument 1: mode/string pointer; observed mode is `rb`;
- result: nonzero pointer-like handle on the successful branch, null on failure branch.

The exact string encoding, path namespace, and lifetime/ownership rules are not yet
proven. The return's exact C type is therefore kept provisional.

## 2. `s3eFileRead`

The caller at `0x13A00E` sets:
- r0 = destination address (`sp + 0x1C`);
- r1 = 4 (item size);
- r2 = 1 (item count);
- r3 = saved file handle.

It then calls veneer `0x30C`, whose A32 instructions calculate the external slot
`0x13B7E0`, matching `s3eFileRead`. Repeated callsites use the same four-register layout.
Other callsites at `0x13A216` and `0x13A226` use element sizes 4 and 2 respectively,
with a 16-bit count loaded into r2 and the handle in r3.

This is strong evidence for the argument order:
`(destination, item_size, item_count, file_handle)`.

Important limitation: the return value is not consumed at the examined callsites.
The guest therefore does not establish whether the API returns bytes, items, a status
code, or another value. Short-read and error behavior remain unverified.

## 3. `s3eFileGetChar`

At `0x139F9E`, r0 is loaded from descriptor offset `+4` and passed to veneer `0xAB0`,
which resolves to slot `0x13BA6C` (`s3eFileGetChar`).
The helper tests the returned value by adding one and checking for zero. A return of
`-1` is converted to the helper's EOF/error path; other values are passed through.
This supports an integer-like return with `-1` as the sentinel in this specific reader.

## 4. `s3eFileClose`

At `0x13A348`, the helper loads the handle from `[r0]`, skips the call if null, then
calls veneer `0xC0`. The veneer resolves to slot `0x13B71C` (`s3eFileClose`).
The stored handle is cleared to zero after the call.

The callsite supports one handle argument. The return value is ignored, so its exact
return type and close-error semantics are not established.

## 5. Veneer resolution

| Veneer | Effective slot | Import |
|---|---:|---|
| `0xA5C` | `0x13BA50` | `s3eFileOpen` |
| `0x30C` | `0x13B7E0` | `s3eFileRead` |
| `0xAB0` | `0x13BA6C` | `s3eFileGetChar` |
| `0xC0` | `0x13B71C` | `s3eFileClose` |

The Stage 78 scan had reported no direct `s3eFileRead`/`s3eFileClose` veneer in its
recovered inventory. This stage resolves the two missing entries by following actual
guest call targets (`0x30C` and `0xC0`) and computing their slots. The earlier absence
was a scan-coverage gap, not evidence that those APIs were unused.

## 6. Candidate API shapes — not approved production prototypes

```c
/* Candidate shapes inferred from guest callsites; return types remain provisional. */
void *candidate_s3eFileOpen(const char *path, const char *mode);
int32_t candidate_s3eFileRead(void *destination,
                             uint32_t item_size,
                             uint32_t item_count,
                             void *file_handle);
int32_t candidate_s3eFileGetChar(void *file_handle);
void candidate_s3eFileClose(void *file_handle);
```

Only argument placement/order is approved for `Open`, `Read`, and `Close`.
For `GetChar`, the integer-like result and `-1` sentinel are also supported by the
guest helper. The candidate `Read`/`Close` return types are deliberately not treated
as proven; the C declarations above are a documentation aid, not an implementation
contract.

## 7. Adapter activation decision

No new file handlers are enabled. Reasons:
- `s3eFileRead` result and short-read semantics are unknown;
- file path mapping into APK assets/private storage is not implemented;
- file-handle ownership and every error cleanup path are not fully audited;
- no runtime guest-to-host file bridge test exists.

The next stage should build a host-side file adapter contract and tests around these
known argument layouts, but keep import binding disabled until path mapping, return
semantics, and lifecycle behavior are validated.

## 8. Runtime status
Static disassembly only. No Android file descriptor/asset manager was opened, no guest
file operation was executed, and the original game has not booted on ARM64.
