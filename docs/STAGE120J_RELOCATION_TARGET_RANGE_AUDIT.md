# Stage 120J — Relocation Target Segment and Memory-Range Audit

## Goal

Validate the address range touched by each known relocation against ELF32 PT_LOAD memory mappings. Distinguish bytes present in the ELF file from zero-filled memory (BSS) without reading or modifying relocation targets.

## Classification

PT_LOAD memory ranges are half-open:
- Memory: [p_vaddr, p_vaddr + p_memsz)
- File-backed: [p_vaddr, p_vaddr + p_filesz)
- Zero-fill/BSS: [p_vaddr + p_filesz, p_vaddr + p_memsz)

Possible statuses:
- file_backed
- zero_fill_bss
- crosses_file_backed_into_bss
- crosses_segment_end
- outside_all_pt_load_segments
- ambiguous_overlapping_pt_load_segments
- unknown_width_file_backed / unknown_width_zero_fill_bss
- no_target_access (R_ARM_NONE)
- non_virtual_target_domain
- target_range_overflows_elf32

## Known ARM relocation widths

Known 32-bit ARM relocation types with a 4-byte target access are explicitly listed in guest_arm_relocation_target_audit.py. Unknown or unverified relocation types are not assigned a guessed width. R_ARM_NONE is represented as zero target access.

## Integration

The existing unified command adds:
- target_access_width to each relocation record;
- target_range classification and matching PT_LOAD segment metadata;
- target_range_audit.target_count;
- target_range_audit.status_counts;
- target_range_audit.targets.

Usage:

    python3 tools/guest_arm_relocation_reconciliation.py /path/to/guest.elf --json /tmp/relocation-reconciliation.json

## Validation

GitHub Actions run 42601733876 passed:
- 188 Python tests.
- File-backed, BSS, file-to-BSS crossing, segment-end crossing, and out-of-range cases.
- Ambiguous overlapping PT_LOAD mappings.
- Unknown relocation width preservation.
- ELF32 address overflow and invalid-width handling.
- R_ARM_NONE no-access behavior.
- Unified RELR target range integration and BSS classification.
- Host C mock compilation and native regression executables.

## Deliberate limitations

- ELF32 EM_ARM only.
- Target addresses are interpreted as link-time virtual addresses; no load bias is calculated.
- The audit does not dereference, read, or modify target bytes.
- It does not apply relocations or infer implicit REL addends.
- Unknown relocation types remain unknown; no guessed width is used.
- This stage does not establish Android runtime compatibility or original game boot.
