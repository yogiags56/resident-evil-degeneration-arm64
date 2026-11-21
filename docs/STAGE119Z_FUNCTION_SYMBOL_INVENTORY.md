# Stage 119Z — Function-Symbol & Relocation Target Inventory

## Goal

Inventory ELF32 ARM STT_FUNC symbols and compare their declared extents with $a/$t/$d mapping-symbol ranges. Join CALL/JUMP relocation references to symbols by exact symbol-table section index and symbol index.

## Usage

    python3 tools/guest_arm_function_inventory.py /path/to/guest.elf --json /tmp/functions.json

## Classification

- arm, thumb, data: function extent overlaps one corresponding mapping range.
- mixed: function extent overlaps more than one mapping class.
- unknown or unmapped: mapping symbols are absent or do not cover the extent.
- zero-size: symbol has no declared extent; only its start can be classified.
- extends-past-section: declared extent exceeds its containing section.
- unresolved, absolute, non-executable, out-of-bounds: symbol is not a normal function in executable section bytes.

For ARM Thumb function symbols, st_value bit zero is retained as a state indicator and cleared before calculating the section offset.

## Relocation correlation

CALL/JUMP references are joined using (symbol table section index, symbol index). This avoids matching ambiguous symbol names. Relocations remain unapplied; no final target address or runtime control-flow edge is inferred.

## Tests

Synthetic ELF fixtures cover ARM/Thumb/data classification, Thumb state bit, mixed-mode extents, zero-size functions, out-of-section extents, undefined symbols, and malformed symbol-table links.
