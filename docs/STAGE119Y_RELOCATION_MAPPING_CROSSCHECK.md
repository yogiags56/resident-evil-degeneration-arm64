# Stage 119Y — Relocation and Mapping-Symbol Cross-Check

## Goal

Cross-reference relocation sites against ELF32 ARM local mapping symbols. The tool classifies each relocation location as ARM code, Thumb code, data, unmapped executable bytes, or non-executable section content.

## Usage

    python3 tools/guest_arm_relocation_crosscheck.py /path/to/guest.elf --json /tmp/crosscheck.json

## Interpretation

- CALL/JUMP relocation types at ARM or Thumb mapping ranges are counted as code-site references.
- CALL/JUMP relocations landing in data, unmapped, or non-executable ranges produce a warning.
- Undefined symbols are labelled undefined/external; this does not identify their provider or prove an S3E import.
- Defined symbols include their section when the section index is ordinary and available.
- Absolute symbols are identified separately.
- No final target address is calculated and no relocation is applied.

## Evidence limits

A relocation describes a link-time fixup, not necessarily the final runtime control-flow edge. Its site classification is only as reliable as the mapping symbols in the supplied ELF. The report does not infer PLT/GOT resolution, dynamic loader behavior, import names, or runtime addresses.

## Tests

Synthetic report fixtures cover ARM/Thumb/data classifications, call/jump warnings outside code, undefined symbol handling, unknown executable ranges, and non-executable relocation targets.
