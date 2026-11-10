# Stage 121A.1 — RELR and Symbol/Dependency Reconciliation

**Status:** source-level reconciliation findings recorded; no branch merge performed.
**Date:** 2026-11-10
**Working branch:** `stage-121a-branch-integration-audit`
**Baseline:** `main` at `a94392490d86cfb627185c963b5134c560cde3c1`

## 1. Two Stage 120Q variants

Compared the PR patches and final file contents for:
- PR #40 / `stage-120q-relr-differential-reconciliation`
- PR #41 / `stage-120q-relr-differential-validation`

The variants share the RELR decoder and table-mapping design but differ in several concrete areas.

### 1.1 RELR target provenance

The reconciliation variant exposes `decode_relr_records()`. Each expanded target retains:
- expanded relocation index;
- encoded table-entry index;
- direct versus bitmap encoding;
- bitmap bit index when applicable;
- target virtual address and numeric target offset.

It retains `decode_relr()` as a compatibility wrapper returning numeric target addresses.

The validation variant exposes only `decode_relr()` returning numeric addresses. Its tests do not retain encoded-entry provenance.

**Reconciliation decision:** preserve the record-oriented API and compatibility wrapper. Provenance is useful when correlating RELR entries with other relocation formats and debugging malformed or overlapping targets.

### 1.2 Independent GNU readelf check

The validation variant adds a synthetic ELF32 ARM fixture containing a RELR section and a differential test that invokes GNU `readelf -rW`, then compares expanded target addresses.

The reconciliation variant's test file does not include this independent-tool check.

**Reconciliation decision:** retain the GNU readelf fixture/test in addition to the provenance API. The test must remain conditional when GNU readelf is unavailable and must report a skip, not a false pass.

### 1.3 Orphan DT_ANDROID_RELRCOUNT

The reconciliation variant rejects `DT_ANDROID_RELRCOUNT` when no RELR address/size/entry-size tags are present.

The validation variant preserves the count as a hint even when no RELR table is present, and both dynamic and APS2 audit outputs expose `android_relr_count_hint`.

**Reconciliation decision:** treat `DT_ANDROID_RELRCOUNT` as non-authoritative metadata. Preserve it in the report even when no RELR table is available; do not use it as a table size or synthesize a table from it. A separate diagnostic may flag the orphan hint, but it should not discard otherwise parseable metadata by itself. The parser must still reject incomplete RELR triplets and conflicting aliases.

### 1.4 Dynamic/APS2 audit output

The validation variant explicitly adds `android_relr_count_hint` to both PT_DYNAMIC and APS2 report objects. The reconciliation variant has the cross-format overlap output but does not expose that top-level hint in those report objects.

**Reconciliation decision:** retain the top-level hint field in both audit outputs alongside cross-format target-overlap reporting.

### 1.5 Test coverage differences

The validation test file adds:
- a synthetic ELF32 ARM RELR fixture;
- a GNU readelf differential check;
- preservation test for `DT_ANDROID_RELRCOUNT` without a table.

The reconciliation test file adds the provenance-related decoder implementation but retains its own fixture coverage. The merged test suite should include both sets, with test names adjusted only if necessary to avoid duplicates.

## 2. Stage 120X / 120Y / 120Z symbol and dependency work

Compared PR #25, #26, and #27 changed-file lists and selected exact file patches/blob identities.

### 2.1 Symbol resolver correction

PR #25 initially sets `name_seen` immediately after matching a symbol name, before filtering out undefined, LOCAL, hidden/internal, or unsupported bindings.

PR #26 and PR #27 move `name_seen` after eligibility checks. This prevents an ineligible symbol with the requested name from causing a misleading version-mismatch result.

The final blob SHA for `runtime/guest/re_guest_symbol_resolve.c` is identical on the inspected 120Y and 120Z tips: `234626f0df2381169453985d99a4058899a366de`.

**Reconciliation decision:** keep the corrected 120Y/120Z resolver ordering; do not regress to the initial 120X ordering.

### 2.2 Stage 120Y symbol-table adapter

PR #26 adds:
- bounded dynamic symbol inventory and ordered `DT_NEEDED` extraction to the PT_DYNAMIC audit;
- `tools/guest_arm_symbol_adapter.py`;
- adapter tests and Stage 120Y documentation/report.

The adapter is explicitly an inventory layer. It does not perform filesystem search, dependency loading, provider compatibility inference, load-bias application, or relocation.

The inspected final blob for `tools/guest_arm_symbol_adapter.py` is identical on 120Y and 120Z: `52713cdb8ed34cdc263c77ff224b2804f3e5f3ab`.

### 2.3 Stage 120Z dependency graph

PR #27 adds:
- `tools/guest_arm_dependency_graph.py`;
- `tests/test_guest_arm_dependency_graph.py`;
- Stage 120Z docs/report and test-runner integration.

The graph builder consumes caller-supplied adapter inventories, rejects duplicate/path-like SONAMEs, reports unresolved dependencies, detects cycles, and emits dependency-first order only for acyclic graphs. It performs no filesystem search, library loading, mapping, relocation, or execution.

The dependency graph file is absent on the 120Y tip and present on the 120Z tip. It must be retained as a distinct Stage 120Z addition.

### 2.4 Shared final file contents

The following inspected file blobs are identical on 120Y and 120Z:
- `tools/guest_arm_dynamic_relocation_segment_audit.py`: `24b02bbeac9efeaac64a3ce57a4ff8591d567080`
- `runtime/guest/re_guest_symbol_resolve.c`: `234626f0df2381169453985d99a4058899a366de`
- `tools/guest_arm_symbol_adapter.py`: `52713cdb8ed34cdc263c77ff224b2804f3e5f3ab`
- `tests/test_re_guest_elf_materialize.c`: `aa7705587d140459f1ad94f58edad9ac8c9a9dbd`

This confirms these exact files agree at the inspected tips; it does not prove that all files or commits are equivalent.

## 3. Candidate integration policy

1. For RELR, use the record-oriented decoder with the compatibility address-only API.
2. Carry the GNU readelf differential fixture/test into the candidate integration.
3. Preserve `DT_ANDROID_RELRCOUNT` as a non-authoritative hint, including when orphaned; do not use it to size or invent a table.
4. Retain Stage 120Y's symbol adapter and corrected resolver eligibility ordering.
5. Retain Stage 120Z's dependency graph and tests.
6. Preserve both PRs #40 and #41 as review history; neither PR is authorized to merge merely because a candidate implementation combines their changes.
7. Do not declare the integration complete until the complete source tree has been assembled and the full host and Android ARM64 workflows have passed on that exact integrated commit.

## 4. Remaining work

- Perform a complete file-by-file reconciliation of the two Q PRs, including APS2 decoder changes and report schemas.
- Inspect all unique commits/files across 120X/Y/Z, not only the selected shared files above.
- Assemble a candidate tree on a separate integration branch with an explicit merge/reconciliation record.
- Run full host tests and Android ARM64 native build on that exact tree.
- Keep `main` unchanged pending review and explicit approval.

## 5. Validation boundary

This document records patch and file-content review. No merge was performed. No combined Stage 120 test run has been claimed. No original game boot or gameplay validation is claimed.
