# Stage 120Q Report — Canonical RELR Decoder and Cross-Audit Parity

Status: implementation committed to branch; CI validation pending.

## Delivered

- Consolidated both RELR audit paths onto tools/guest_arm_relr.py.
- Added metadata-preserving decode_relr_records() and retained the address-only compatibility wrapper.
- Aligned empty-bitmap and zero-length table behavior across audits.
- Enabled Android and mixed standard/Android RELR aliases in the standalone audit and reconciliation path.
- Added decoder-parity and Android reconciliation regression tests.

## Important distinction

This stage establishes internal consistency between report paths. It does not yet claim independent differential validation against readelf, Bionic binaries, or a separately implemented decoder.

## Safety

Read-only ELF32 EM_ARM analysis. No relocation application, target-memory read/write, load-bias calculation, or game boot is claimed.
