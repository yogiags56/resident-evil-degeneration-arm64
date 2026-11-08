# Stage 89 — s3eFileRead ABI Contract Closure (Conservative)

## Confirmed from original guest evidence
- AAPCS32 argument words are passed in r0-r3.
- r0 is destination guest address.
- r1 is item size.
- r2 is item count.
- r3 is file-handle-like value.
- Recovered examples include `(size=4,count=1)` and further calls with item sizes 4 and 2.
- The examined callers do not consume the return value.

## What this stage can safely close
The new capture module records the four argument words and calculates `item_size * item_count` in uint64. It rejects zero size/count and null destination/handle before any operation. It intentionally does not dereference guest memory, read from an asset, mutate guest registers, or synthesize a return value.

## Still unresolved
- Whether the return is bytes, complete items, status, or another value.
- Whether short reads return partial item counts, bytes, or zero.
- Error convention and interaction with EOF.
- Whether `item_size == 0` or `item_count == 0` have defined special behavior in the original SDK.
- Exact original SDK version/API contract documentation has not been recovered.

## Activation decision
**No `s3eFileRead` guest import handler is enabled.** The return authorization function deliberately returns false. The Stage 82 byte reader remains an internal host adapter primitive and must not be confused with the guest API return contract.

## Test scope
Host C11 strict build and UBSan cover argument capture, multiplication, null/zero guards, and the explicit disabled-return gate. These tests do not establish original Marmalade runtime semantics and do not boot the game.
