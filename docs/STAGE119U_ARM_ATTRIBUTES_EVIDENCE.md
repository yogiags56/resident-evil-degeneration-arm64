# Stage 119U — ARM Attribute Scopes and External Evidence Capture

## Scope

Extend the Stage 119T parser using the Arm ABI's typed attribute grammar. The parser retains file-, section-, and symbol-scope public AEABI attributes; handles the special Tag_compatibility payload (ULEB128 flag plus vendor NTBS); recognizes Tag_nodefaults as argument-free; and preserves unknown vendor-private payload bytes as opaque data.

Add tools/guest_arm_elf_evidence.py to capture versioned output from GNU readelf or LLVM llvm-readobj. The command uses argument arrays (no shell), timeout, and records source SHA-256, tool path/version, exit code, stdout, and stderr. It never executes the target ELF.

## Usage

    python3 tools/guest_arm_elf_audit.py /path/to/guest.elf --json /tmp/guest-elf.json
    python3 tools/guest_arm_elf_evidence.py /path/to/guest.elf --tool auto --json /tmp/guest-evidence.json

Keep proprietary binaries outside the repository. Evidence output may contain paths and symbols; inspect it before sharing.

## ABI basis

The parser follows the public syntax in AAELF32 §5.3.6 and Addenda32: vendor subsections may contain file, section, and symbol sub-subsections; public tags use ULEB128 values or NUL-terminated strings, with defined special encodings. Private vendor payloads remain opaque. The public AAELF32 and Addenda32 specifications are linked in the accompanying engineering notes.

## Limits

The parser remains intentionally constrained: conventional ELF section tables only; no extended section numbering; no attribute inheritance/merge algorithm; no full semantic validation of every enum or linker compatibility rule. Binary attributes are metadata claims, not disassembly or proof of runtime reset state. External tool output is raw evidence and its exit code must be reviewed.

## Tests

Synthetic fixtures cover file/section/symbol scopes, Tag_compatibility, Tag_nodefaults, Tag_conformance, private vendor payload preservation, malformed bounds, and endianness. Mocked evidence-tool tests cover tool selection, timeout, source hashing, output capture, and missing-tool handling. Host CI is the execution authority.
