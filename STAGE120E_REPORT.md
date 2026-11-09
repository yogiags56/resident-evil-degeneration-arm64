# Stage 120E Report — Android APS2 Packed Relocations

**Status:** Implemented; host validation recorded in this commit.

## Delivered

- Added `tools/guest_arm_aps2_relocation_audit.py`.
- Added a bounded signed 32-bit SLEB128 decoder and APS2 REL/RELA group decoder.
- Added PT_DYNAMIC tag discovery for Android APS2 relocation streams.
- Added PT_LOAD file-backed mapping, optional bounded SysV dynamic-symbol resolution, JSON reporting, and safety limits.
- Added synthetic regression coverage in `tests/test_guest_arm_aps2_relocation_audit.py`.
- Added design and evidence boundaries in `docs/STAGE120E_ANDROID_APS2_PACKED_RELOCATIONS.md`.

## Validation

Run:

```bash
python3 -m unittest discover -s tests -p 'test_guest_arm_aps2_relocation_audit.py' -v
python3 -m unittest discover -s tests -v
bash tools/run_tests.sh
```

The first command is the focused Stage 120E test. The latter commands are the full Python and native host regression suites. Results must be recorded from an actual run; this report does not claim Android device execution or successful game boot.

## Evidence boundaries

This stage decodes and inventories packed relocation records only. It does not apply relocations, dereference target addresses, calculate load bias, activate imports, infer shared-library providers, or boot the game. GNU hash and RELR are deferred.
