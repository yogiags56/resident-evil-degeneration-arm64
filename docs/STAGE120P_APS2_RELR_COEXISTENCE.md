# Stage 120P — APS2/RELR Coexistence and Dynamic Relocation Cross-Checks

## Objective

Extend the ELF32 EM_ARM dynamic relocation inventory so Android APS2 REL/RELA and standard/Android RELR can be examined in the same section-header-independent PT_DYNAMIC audit. This is an offline, read-only analysis stage.

## Reference behavior

The RELR expansion follows Android Bionic's linker implementation:
- An even entry is a direct relocation target address; the next bitmap base is address + 4 bytes.
- An odd entry is a bitmap for up to 31 subsequent 32-bit words. Bitmap bit 1 denotes the current base; each following bit denotes the next word.
- After a bitmap, the base advances by 31 words (124 bytes) for ELF32.
- Bionic accepts both standard DT_RELR/DT_RELRSZ and Android DT_ANDROID_RELR/DT_ANDROID_RELRSZ aliases. DT_RELRENT and DT_ANDROID_RELRENT describe the 4-byte entry width.
- DT_ANDROID_RELRCOUNT is retained as a hint only; it is not used as the table size or relocation count.

Primary references:
- Bionic ELF constants: https://android.googlesource.com/platform/bionic/+/main/libc/include/elf.h
- Bionic RELR decoding and APS2/RELR ordering: https://android.googlesource.com/platform/bionic/+/master/linker/linker.cpp
- Bionic's earlier explicit 32-bit bitmap step: https://android.googlesource.com/platform/bionic/+/9da992c2714205a7643c4b2f2aaaa0627e061914/linker/linker.cpp

Bionic processes packed Android relocations and RELR as distinct streams. This audit inventories both; it does not reproduce linker application order or apply relocations.

## Implementation

- Added tools/guest_arm_relr.py with a bounded ELF32 RELR decoder.
- Supports little-endian and big-endian ELF32 words.
- Validates direct-address alignment, monotonic targets, bitmap ordering, word-sized table length, cursor/address overflow, and table bounds.
- Caps the input table at 64 MiB and expanded targets at 4,000,000.
- Accepts standard and Android RELR tags, including mixed aliases, while rejecting missing/conflicting tag values.
- Retains optional DT_ANDROID_RELRCOUNT as android_relr_count_hint without trusting it as a size.
- Integrates RELR inventories into both the ordinary dynamic REL/RELA audit and the APS2 audit.
- Reports target-address collisions across distinct relocation formats instead of silently deduplicating them.

## Tests

- Direct address followed by sparse bitmap and another direct address.
- Bitmap cursor advancement across all 31 ELF32 bitmap positions.
- Little-endian and big-endian decoding.
- Empty table, partial word, unaligned direct address, bitmap-before-base, backwards/overlapping addresses, address/cursor overflow, and unsupported word size.
- Standard RELR tags, Android RELR aliases, mixed alias tags, identical dual aliases, conflicting aliases, missing tags, invalid entry size, and malformed table size.
- Optional Android RELR count hint and orphan-hint rejection.
- Expanded target-count safety bound.
- APS2 REL and RELA coexistence with RELR.
- Cross-format duplicate target reporting.

## Output and safety contract

Reports contain relocation target addresses only. The parser never dereferences target contents, never writes to the ELF, never computes runtime load bias, and never applies relocations. Cross-format overlaps are diagnostic evidence, not automatic proof of a valid or invalid runtime image.

## Remaining work

- Differentially compare decoder output with an independent RELR implementation or toolchain-produced ELF fixture.
- Cross-check DT_ANDROID_RELRCOUNT behavior across Android API levels.
- Analyze mixed standard/Android alias behavior against more Bionic revisions.
- Continue toward real ARM64 build and device-side guest execution only after the static ELF evidence is complete.

## Validation evidence

GitHub Actions run 42618941160 passed 272 Python tests in 12.026 seconds and host C/native regressions. The validation covers standard and Android RELR tags, mixed aliases, 32-bit bitmap expansion, bounds, malformed inputs, coexistence with APS2 REL/RELA, and cross-encoding target collision reporting. See STAGE120P_VALIDATION_LOG.txt.
