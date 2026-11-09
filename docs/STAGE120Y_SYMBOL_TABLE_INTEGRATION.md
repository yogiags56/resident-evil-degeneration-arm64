# Stage 120Y — ELF Symbol Table Integration

## Objective
Connect the bounded PT_DYNAMIC symbol/version audit to the native symbol
resolver through an explicit JSON adapter inventory.

## Changes
- Dynamic audit reports now expose the complete bounded DT_SYMTAB inventory.
- DT_NEEDED strings are resolved through bounded DT_STRTAB offsets and kept
  in original dynamic-entry order.
- The adapter separates eligible defined provider symbols from undefined
  GLOBAL/WEAK lookup requests.
- Version requirement names and required library SONAMEs are retained.
- Provider values remain ELF virtual addresses; the verified provider load
  bias must be applied by a later runtime integration layer.

## Safety boundary
No library is opened or loaded. No search path, host symbol table, dependency
scope, version compatibility, or load bias is inferred. The adapter does not
apply relocations. Local and hidden/internal symbols are not exported as
external providers or lookup requests.

## Validation
Python fixture tests cover dependency ordering, provider/request separation,
version constraints, invalid descriptors, and missing symbol bounds.
