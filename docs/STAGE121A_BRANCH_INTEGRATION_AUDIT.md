# Stage 121A — Branch Integration Audit

**Status:** audit recorded; no Stage 120 branch has been merged.
**Audit date:** 2026-11-10
**Repository:** `yogiags56/resident-evil-degeneration-arm64`
**Audit base:** `main` at `a94392490d86cfb627185c963b5134c560cde3c1`

## 1. Scope and method

Compared each Stage 120 branch against `main` using GitHub's compare endpoint. The returned comparisons report a common merge base equal to the audited `main` commit and zero commits behind for each branch. The ahead counts below are comparison metadata, not proof that the branch contains every change from another Stage 120 branch.

## 2. Stage 120 branch inventory

| Branch | Ahead of main |
|---|---:|
| 120A dynamic import inventory | 103 |
| 120B PLT/GOT cross-reference | 109 |
| 120C PT_DYNAMIC parser | 111 |
| 120D dynamic relocation mapping | 114 |
| 120E APS2 packed relocations | 141 |
| 120F dynamic relocation reconciliation | 141 |
| 120G GNU hash symbol bounds | 168 |
| 120H ELF symbol versioning | 184 |
| 120I ELF32 RELR decoding | 197 |
| 120J relocation target range audit | 219 |
| 120K ARM relocation ABI semantics | 236 |
| 120L relocation contract cross-validation | 252 |
| 120M independent ELF cross-check/copy size | 277 |
| 120N packed dynamic symbol metadata parity | 299 |
| 120O APS2 conformance hardening | 319 |
| 120P APS2/RELR coexistence | 373 |
| 120Q RELR differential reconciliation | 393 |
| 120Q RELR differential validation | 395 |
| 120R dynamic RELR differential | 402 |
| 120S ELF32 load plan | 417 |
| 120T guest map transaction | 429 |
| 120U ELF segment materialization | 442 |
| 120V guest permission finalization | 452 |
| 120W relocation application engine | 462 |
| 120X dynamic symbol resolution | 473 |
| 120Y ELF symbol table integration | 486 |
| 120Z dependency graph resolution | 499 |

## 3. Confirmed topology findings

1. All 27 Stage 120 branches compared as `ahead` of the audited `main`, with `behind=0` and the same merge base. Their histories are therefore rooted at the same baseline, but their tips are not necessarily a single linear chain.
2. 120E and 120F compare as `identical` (zero ahead/behind; same tip SHA `5c0b671a8d95e24dedc7dd4a8525b094e85cf203`).
3. The two 120Q variants are divergent. Comparing `stage-120q-relr-differential-reconciliation` to `stage-120q-relr-differential-validation` reports 23 commits on the head side and 21 on the base side, with merge base `9ceb04b7aadaa94022114e7d78d30af3d96fa5ad`.
4. 120X and 120Y are divergent (17 ahead / 4 behind; merge base `dfb22f15a0fb2149267eb7328d57d9385de01fdf`).
5. 120Y and 120Z are divergent (19 ahead / 6 behind; merge base `aa7c3b53797951519a7a148cd9442b5641f12f17`). 120Z adds the dependency-graph implementation and its test, while 120Y does not contain `tools/guest_arm_dependency_graph.py`.
6. Spot-checking selected file blobs on 120Y and 120Z found identical blob SHAs for `tools/guest_arm_dynamic_relocation_segment_audit.py`, `runtime/guest/re_guest_symbol_resolve.c`, `tools/guest_arm_symbol_adapter.py`, and `tests/test_re_guest_elf_materialize.c`. This is evidence that those exact file contents agree at the two tips; it is not a complete conflict audit.

## 4. Integration implications

- Do not merge all 27 branches in alphabetical order.
- Treat 120E/120F as one identical tip for integration planning; retain both branch names/PRs for traceability.
- Reconcile the two 120Q variants before choosing the canonical RELR differential result.
- Reconcile 120X/120Y/120Z histories explicitly. The dependency graph work in 120Z must be preserved, while any distinct commits on the 120Y side must be reviewed rather than silently discarded.
- Before integration, compare changed-file patches and inspect the complete final contents of overlapping files, especially:
  - `tools/guest_arm_dynamic_relocation_segment_audit.py`
  - `tools/guest_arm_relr.py` and related RELR tests
  - `tools/guest_arm_symbol_adapter.py`
  - `runtime/guest/re_guest_symbol_resolve.c`
  - `tools/guest_arm_dependency_graph.py`
  - `runtime/guest/re_guest_elf_materialize.c` and its tests
  - `.github/workflows/ci.yml`, `tools/run_tests.sh`, `README.md`, `ROADMAP.md`, and `docs/STATUS.md`
- Preserve the independent validation logs and source-manifest/checksum behavior when assembling the integrated tree.

## 5. Recommended next steps

1. Complete a file-level three-way reconciliation for the 120Q pair.
2. Complete a file-level three-way reconciliation for 120X/120Y/120Z, identifying commits and files unique to each side.
3. Establish a canonical Stage 120 integration snapshot on a dedicated branch, with an explicit record of selected commits and any manually reconciled files.
4. Run the full host test suite and Android ARM64 native build on that combined snapshot. Individual branch CI results do not prove combined integration correctness.
5. Only after the combined snapshot passes should Stage 121B begin wiring the unified ELF load and relocation pipeline into the runtime.

## 6. Safety and validation boundary

This audit did not merge branches, change `main`, alter open pull requests, or claim that the original game boots or runs. It records GitHub branch topology and selected file-level observations only. A passing per-branch workflow is not a substitute for testing the integrated tree.
