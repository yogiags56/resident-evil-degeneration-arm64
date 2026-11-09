# Stage 120F — Dynamic Relocation Reconciliation

## Goal
Create a read-only reconciliation view across section-backed ELF32 ARM SHT_REL/SHT_RELA records supported by Stage 119X, PT_DYNAMIC REL/RELA/JMPREL records from Stage 120D, and Android APS2 records from Stage 120E.

## Usage
    python3 tools/guest_arm_relocation_reconciliation.py /path/to/guest.elf --json /tmp/relocation-reconciliation.json

## Reconciliation rules
Each record retains target domain/address, relocation type, symbol index, addend, source table, and symbol-table namespace. File-backed table ranges retain file offset and byte length.

- Exact duplicate records require the same target, type, symbol index, addend, and symbol-table namespace.
- Same numeric tuples from different symbol-table namespaces are candidates only; index equality does not prove symbol identity.
- Exact and partial file-range overlaps are reported independently from semantic matches.
- No records are removed or silently deduplicated. Raw counts include every source declaration.
- REL implicit addends remain unknown (null); target bytes are not read to reconstruct them.
- Section-backed parsing uses the existing conventional SHT_SYMTAB implementation. Unsupported section/symbol-table combinations are marked unavailable while PT_DYNAMIC inventories remain available.

## Validation
Synthetic tests cover exact duplicate declarations, partial overlap, namespace ambiguity, differing addends, integrated APS2 input, and invalid ELF input. Run bash tools/run_tests.sh.

## Limitations
ELF32 EM_ARM only. GNU hash, RELR, runtime load bias, relocation application, provider resolution, and game boot are outside this stage. A green host suite does not prove the proprietary game binary contains any particular relocation encoding.
