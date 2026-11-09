# Stage 120N Report — Packed and Dynamic Relocation Metadata Parity

Status: implementation committed; CI validation pending.

## Delivered

- APS2 relocation entries now preserve symbol size, definition state, section index, and version metadata when bounded dynamic-symbol metadata is available.
- Missing symbol bounds remain explicitly unresolved.
- Unified reconciliation now assigns a proven DT_SYMTAB namespace only when the relevant dynamic-symbol inventory resolved successfully.
- APS2 R_ARM_COPY width can use st_size only when the symbol is defined, nonzero-indexed, positive-sized, and namespaced.
- Corrected APS2 decoding according to the Android Bionic implementation: absolute r_info values, group-wide info reuse, grouped addend delta applied once, per-entry addend deltas, and zero addend when absent.
- Added synthetic tests for metadata provenance, COPY target width, grouped/ungrouped r_info, and addend behavior.

## Usage

    python3 tools/guest_arm_aps2_relocation_audit.py guest.elf --json /tmp/aps2.json
    python3 tools/guest_arm_relocation_reconciliation.py guest.elf --json /tmp/relocation-inventory.json

## References

- Android Bionic: https://android.googlesource.com/platform/bionic/+/refs/heads/main/linker/linker_reloc_iterators.h
- LLVM APS2 format: https://reviews.llvm.org/D39152

## Safety

Metadata inspection only. No relocation is applied, no target bytes are dereferenced, and no provider is inferred.
