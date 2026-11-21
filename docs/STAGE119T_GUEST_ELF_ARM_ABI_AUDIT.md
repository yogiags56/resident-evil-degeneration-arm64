# Stage 119T — Guest ELF and ARM ABI Metadata Audit

## Objective

Add a read-only tool that inspects a locally supplied ELF and reports ELF class, byte order, machine, flags, section names, and selected AEABI .ARM.attributes. The proprietary game executable is not in this repository, so synthetic fixtures validate the parser without making claims about the game's actual binary.

## Usage

    python3 tools/guest_arm_elf_audit.py /path/to/guest.elf --json /tmp/guest-elf-report.json

Keep the original executable outside the repository. The tool reads bytes only; it does not execute, modify, extract, or redistribute the input.

## Interpretation

- e_machine=40 identifies ARM; e_machine=183 identifies AArch64.
- .ARM.attributes may include Tag_CPU_arch, Tag_CPU_arch_profile, Tag_THUMB_ISA_use, Tag_FP_arch, Tag_Advanced_SIMD_arch, and Tag_ABI_VFP_args.
- Absent attributes are unknown, not proof that a feature is absent.
- ELF metadata does not establish runtime SCTLR reset state, MMU configuration, or privilege behavior.
- This stage does not disassemble executable sections or inventory actual A32/T32/VFP/NEON instructions.

## Parser limits

The implementation supports conventional ELF32/ELF64 files with a non-empty section table and a single AEABI file-attribute subsection. Extended section numbering and unknown ARM attribute subsection tags fail closed. Known string-valued tags are decoded explicitly; other tags are read as ULEB128. This is an initial constrained parser, not a complete ARM ABI implementation.

## Validation

Synthetic tests cover ARM32 little- and big-endian ELF, CPU/Thumb/FP/SIMD tags, missing attributes, invalid ELF magic, truncated program-header bounds, invalid section bounds, and malformed attribute data. CI is the authoritative execution record; no local test execution is claimed.

## Next

1. Validate remaining ARM attribute tag types against the ABI specification.
2. Add optional version-recorded readelf/llvm-readobj evidence capture.
3. When a permitted original executable is supplied locally, record its hash and actual metadata report without committing the binary.
4. Add a pinned disassembler for ARM/Thumb instruction inventory.
5. Keep runtime system-control/reset semantics unknown until runtime evidence supports them.
