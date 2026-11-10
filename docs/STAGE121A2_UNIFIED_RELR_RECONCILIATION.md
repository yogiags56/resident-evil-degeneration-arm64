# Stage 121A.2 — Unified RELR Reconciliation

**Status:** implementation committed on an isolated Stage 120Q-based branch; CI/test execution not yet verified.
**Base:** `stage-120q-relr-differential-reconciliation`
**Branch:** `stage-121a2-relr-unified-base-q40`

## Implemented

1. Retained the record-oriented ELF32 RELR decoder API `decode_relr_records()`.
2. Retained the compatibility API `decode_relr()`, which returns numeric target addresses.
3. Updated `collect_relr_tables()` to derive its legacy `targets` array from decoded records and expose the richer `target_records` array. Each record carries expanded index, encoded table-entry index, encoding kind, optional bitmap bit, target address, and numeric target offset.
4. Preserved `DT_ANDROID_RELRCOUNT` as a non-authoritative hint even when no RELR table exists. It is not used to infer table size or synthesize entries.
5. Exposed `android_relr_count_hint` in the PT_DYNAMIC and APS2 audit report roots.
6. Carried the validation-variant RELR tests, including the GNU `readelf -rW` differential fixture, into the Q-reconciliation base.
7. Added tests for direct/bitmap provenance and propagation through `collect_relr_tables()`, while retaining the compatibility target list.

## Compatibility

- Existing `targets` report field remains unchanged.
- New `target_records` field is additive.
- Existing `decode_relr()` callers continue to receive `list[int]`.
- An orphan `DT_ANDROID_RELRCOUNT` no longer causes the whole RELR collection to fail; the hint remains report metadata.
- Incomplete RELR address/size/entry-size triplets and conflicting standard/Android aliases remain errors.

## Branch base and scope

This implementation branch is based on `stage-120q-relr-differential-reconciliation`, not `main`. That base is deliberate: the earlier Stage 121A documentation branch was rooted at `main`, where the Stage 120Q implementation files are absent. A source-only branch rooted at main would not be a valid runnable integration snapshot.

The separate `stage-121a-branch-integration-audit` branch remains documentation-only. No Stage 120 PR was merged and `main` was not modified.

## Verification still required

- Run the complete Python source-check/test entrypoint on this exact branch tip.
- Confirm the GNU readelf differential test runs (or explicitly skips if the binary is unavailable).
- Run Android ARM64 native build workflow on this exact integrated branch.
- Add/verify end-to-end assertions that PT_DYNAMIC and APS2 JSON reports both contain `target_records` inside each RELR table and preserve `android_relr_count_hint` at the report root.
- Inspect all workflow results on this exact branch tip before calling Stage 121A.2 green.

No test pass, Android build pass, device boot, or gameplay validation is claimed by this document.
