# Stage 119W — ELF ARM Mapping Symbols and Mode-Aware Inventory

## Goal

Use ELF local mapping symbols to distinguish ARM code ($a), Thumb code ($t), and data ($d) inside executable sections. This removes the global-mode assumption for sections whose mapping symbols are present.

## Usage

    python3 tools/guest_arm_instruction_inventory.py /path/to/guest.elf --json /tmp/inventory.json

If an executable section has no recognized mapping symbols, the tool requires an explicit fallback:

    python3 tools/guest_arm_instruction_inventory.py /path/to/guest.elf --mode thumb --json /tmp/inventory.json

The fallback applies only to sections with no mapping symbols. It never overrides mapping-symbol classifications. Unknown bytes before the first mapping symbol are reported as unmapped, not guessed.

## Mapping symbols

- $a and $a.<suffix>: ARM instruction range.
- $t and $t.<suffix>: Thumb instruction range.
- $d and $d.<suffix>: data range.
- Only local binding symbols are treated as mapping symbols.
- ET_REL values are section-relative; ET_EXEC/ET_DYN values are converted from virtual address to section-relative offset.
- Duplicate same-kind symbols at one offset collapse; conflicting kinds at one offset fail closed.

## Accounting

The report separates decoded bytes, decoder-undecoded bytes, $d data bytes, and unmapped bytes. Data and unmapped ranges are not sent to Capstone. Each decoded range records its source mapping symbol and selected decoder mode.

## Limits

ELF32 EM_ARM only. Extended symbol section indices, stripped/no-symbol binaries, ELF64/AArch64, relocations, Thumb IT semantic checks, and full control-flow analysis remain outside this stage. A section without mapping symbols must be classified only with an explicit fallback mode. No input is executed or modified.

## Tests

Synthetic ELF fixtures cover mixed ARM/Thumb/data ranges, unknown prefixes, missing mapping symbols, explicit fallback, and conflicting symbols. The decoder remains mocked for deterministic byte-accounting tests.
