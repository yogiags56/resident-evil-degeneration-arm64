# Stage 125F — Original DSO dependency and provider-closure audit

## Purpose

Create a deterministic, fail-closed static audit connecting the hash-pinned
Stage 123O original ELF32 manifest to the guest provider inventories already
implemented. This is a validation gate, not a new loader or a claim that every
dependency is executable.

## Direct DT_NEEDED policy

The policy records all six direct dependencies in the exact ELF order:

1. libc.so
2. libm.so
3. liblog.so
4. libstdc++.so
5. libz.so
6. libdl.so

Each SONAME must have exactly one explicit policy entry. The current libm
classification is deliberately **dependency-only**: no synthetic math exports
are declared and no relocation-backed math symbol is assigned to libm.
The original relocation-backed undefined-name inventory has no direct
libm-specific math symbol; its 21 __aeabi_* helper imports are classified
separately under the RTABI32 guest helper provider. This does not mean libm.so
has been loaded or that the direct dependency is already satisfied at runtime.
A verified guest DSO or explicit compatible runtime strategy is still needed
before production activation.

## Relocation-backed provider coverage

The machine-readable policy lists only adapters implemented in this tree:

- libc.so: current partial guest libc provider set
- liblog.so: two Android log imports
- libstdc++.so: four C++ allocation operators, sharing the libc heap
- libz.so: four inflate-family imports
- libdl.so: four guest dynamic-loader bridge imports
- ARM EABI family: 21 helpers, without inventing a DT_NEEDED SONAME

The audit cross-checks every declared provider symbol against the exact
Stage 123O relocation-backed unique undefined-name inventory. It reports
uncovered required imports and unexpected declared exports. It rejects
duplicate provider ownership, duplicate SONAME policy entries, missing
DT_NEEDED policies, and any attempt to turn dependency-only libm into an
unverified symbol provider.

## Expected current result and fail-closed behavior

The current provider set is intentionally incomplete. The report must keep
production_loader_gate false while relocation-backed imports such as the
remaining filesystem, process/time, pthread, and socket functions are
uncovered. In particular, pthread_create and socket remain explicit blockers.
The audit exits with status 2 when closure is incomplete; that is an expected
result for the current development inventory, not a test-suite failure.

## Files

- reports/STAGE125F_PROVIDER_POLICY.json — explicit SONAME and ABI-family policy
- tools/audit_original_dso_closure.py — deterministic static coverage auditor
- tests/test_original_dso_closure_audit.py — regression tests for coverage and
  fail-closed policy

Run locally:

```sh
python3 tools/audit_original_dso_closure.py \
  reports/STAGE123O_LIBJIROBIODG_PROVIDER_MANIFEST.json \
  reports/STAGE125F_PROVIDER_POLICY.json \
  --json build/original-dso-provider-closure.json
```

## Limitations

This stage does not load or map a system DSO, apply relocations, validate
Android Bionic ABI behavior, or execute constructors. It does not resolve
runtime GLES1/GLES2 fallback libraries. The original game has not booted.
