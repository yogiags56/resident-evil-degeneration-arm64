# Stage 120Q — Canonical RELR Decoder and Cross-Audit Parity

## Objective

Remove semantic drift between the standalone RELR audit and the decoder already used by the dynamic/APS2 audits. Keep one ELF32 RELR decoding implementation, preserve encoded-entry provenance, and ensure Android aliases work through full relocation reconciliation.

## Findings from Stage 120P review

The repository contained two independent RELR decoders:
- tools/guest_arm_relr.py, used by the dynamic relocation and APS2 audits.
- tools/guest_arm_relr_audit.py, used by the broader relocation reconciliation path.

They had diverged. The standalone decoder rejected an odd bitmap word with no set relocation bits (entry value 1), rejected a zero-length table, and only recognized standard DT_RELR tags. Those behaviors disagreed with the shared decoder and reduced compatibility with Android-tagged ELF inputs.

## Implementation

- Added decode_relr_records() as the canonical metadata-preserving expansion routine.
- Kept decode_relr() as a compatibility wrapper returning target addresses.
- Updated the standalone audit to use collect_relr_tables() and decode_relr_records(); no duplicate RELR decoding algorithm remains.
- Added support for standard, Android, and mixed standard/Android RELR dynamic tag aliases in the standalone audit.
- Preserved table-entry index, direct/bitmap encoding, and bitmap-bit provenance in each expanded record.
- Allowed an empty bitmap (word value 1); it advances the bitmap cursor by 31 ELF32 words but contributes no relocation target.
- Allowed a zero-sized RELR table and reports zero encoded entries/targets.
- Updated full reconciliation to detect Android DT_ANDROID_RELR, DT_ANDROID_RELRSZ, and DT_ANDROID_RELRENT.
- Added tests comparing standalone audit records with the canonical decoder output and testing Android alias integration through reconciliation.

## Validation boundaries

The parity test compares two report paths that now share one canonical decoder. It is an internal consistency test, not an independent implementation differential. A later stage should still compare against readelf/Bionic/toolchain-generated fixtures where available.

All parsing remains read-only. RELR targets are relative relocation locations; no target bytes, implicit addends, runtime load bias, or relocated values are inferred.

## Test coverage

- Existing little-endian and big-endian ELF32 fixtures.
- Canonical record parity including table-entry provenance.
- Empty bitmap and bitmap cursor advancement.
- Zero-sized RELR table.
- Standard, Android, and mixed RELR tag aliases.
- Full reconciliation path for Android RELR.
- Existing malformed table, alignment, mapping, and overflow cases.

## Next work

1. Obtain independent toolchain-produced ELF32 RELR fixtures and compare target expansions.
2. Compare the decoder against Bionic/readelf behavior for empty bitmap and cursor-boundary cases.
3. Preserve exact Android tag behavior by API/Bionic revision.
4. Continue relocation-source reconciliation without discarding raw records.
