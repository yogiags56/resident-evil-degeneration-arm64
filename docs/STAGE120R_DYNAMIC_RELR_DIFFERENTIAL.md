# Stage 120R — PT_DYNAMIC RELR Differential Validation

## Objective

Extend Stage 120Q's independent GNU readelf check from a section-only decoder
fixture to the section-header-independent ELF32 ARM dynamic RELR path. The
same encoded bytes are exposed through PT_DYNAMIC/PT_LOAD and SHT_RELR:
the project audit reads the dynamic tags, while GNU readelf provides an
independent section-backed oracle.

## Implementation

- Add a generated ELF32 EM_ARM DYN fixture with PT_LOAD, PT_DYNAMIC, DT_RELR,
  DT_RELRSZ, DT_RELRENT, and a matching SHT_RELR section.
- Assert the dynamic parser expands direct and bitmap entries to the exact
  four expected target addresses.
- Compare the expanded addresses against GNU readelf -rW output.
- Exercise the unified relocation reconciliation path and assert RELR records
  retain relative-only identity: no fabricated symbol index or addend.

## Validation criteria

- Python test discovery includes the new Stage 120R regression module.
- GNU readelf output and PT_DYNAMIC decoder output match exactly when readelf
  is installed; otherwise the external-tool check reports a skip.
- Full Python, host C mock compilation, and native C regression checks pass.
- No source ELF or proprietary game assets are modified or included.

## Scope limits

This is static, read-only ELF32 EM_ARM validation. It does not apply
relocations, read target contents, calculate load bias, execute Android code,
or claim original game boot.
