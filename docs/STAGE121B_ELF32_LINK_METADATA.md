# Stage 121B — Bounded ELF32 ARM dynamic-link metadata

## Goal
Decode the metadata needed by a later dependency resolver without mixing
parsing, symbol policy, address relocation, memory mutation, or guest execution.

## Implemented
- Added a bounded read-only PT_LOAD-backed virtual-address to file-offset
  translator for dynamic metadata.
- Decodes the SysV DT_HASH header and uses nchain as the exact dynamic-symbol
  count bound. The count is capped at 4096.
- Decodes ELF32 dynamic symbol records, including name, value, size, binding,
  type, visibility, and section index.
- Decodes bounded REL and RELA records and the DT_JMPREL/DT_PLTRELSZ/DT_PLTREL
  PLT relocation table.
- Checks dynamic tag uniqueness/presence, ABI entry sizes, table bounds,
  string termination, symbol-name capacity, output capacities, and relocation
  symbol indices.
- Rejects GNU-hash-only discovery and unsupported relocation encodings
  explicitly. No format is guessed.
- Adds a synthetic host regression test and compiles the decoder into the
  Android ARM64 native target.

## Deliberate limits
This is not a dynamic linker. It does not discover dependencies, map provider
ELFs, implement Android linker namespaces or symbol scopes, interpret GNU
symbol versions, resolve weak/strong definitions across DSOs, calculate load
bias, apply relocations, run constructors, or execute game code. The caller
must provide the already parsed image and PT_DYNAMIC descriptor. SysV DT_HASH
is currently required to establish the bounded symbol count; GNU-hash-only
ELFs are reported as unsupported.

## Validation
The synthetic test covers a SysV hash, three symbol slots, bounded symbol-name
decoding, one REL record, output-capacity rejection, and out-of-range table
rejection. ## Validation results
- Host regression suite, including `test_re_guest_elf32_link`: PASS on commit `daa1069b2099dc00d0c502447d0677b7db07a373` (workflow run `42815318804`).
- Android ARM64 native build and ELF verification: PASS on commit `21ddc4a50b53629e198a72d857e706ec2eade99e` (workflow run `42815274644`). The runtime decoder and Android CMake source integration are identical in the validated host and Android commits; the later host-only commit corrected the synthetic test fixture offsets.
- Android artifact uploaded as workflow artifact `resident-evil-degeneration-arm64-stage118-runtime-foundation`, artifact ID `13763829438`.

Stage 121B validation is complete. This remains a metadata decoder, not a dependency loader or playable game runtime.
