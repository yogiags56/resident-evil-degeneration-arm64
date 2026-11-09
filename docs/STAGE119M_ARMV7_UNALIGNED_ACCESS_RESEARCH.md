# Stage 119M — ARMv7 unaligned data-access research

## Purpose

Resolve the provisional strict-alignment policy inherited by the Stage 119J CPU data-memory facade before migrating any production load/store decoder. This stage is architecture research and a decision record; it intentionally does not change runtime behavior.

## Primary references

1. Arm, *ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition*, ARM DDI 0406C.d:
   - Application Level Architecture, A3.2.1, "Unaligned data access" (table A3-1 in the C edition; table numbering differs across revisions).
   - System-level memory access rules in B2.4.5, "Unaligned memory accesses".
   - Official document: https://documentation-service.arm.com/static/5f8daeb7f86e16515cdb8c4e
2. Arm, *Arm Compiler for Embedded User Guide*, "Alignment support in Arm Compiler for Embedded 6":
   https://support.arm.com/documentation/100748/0624/Alignment-support-in-Arm-Compiler-for-Embedded-6/Aligned-and-unaligned-accesses
3. Linux kernel documentation, "Memory alignment" (useful for Linux user-space fault/fix-up behavior, not a replacement for the ARM architecture specification):
   https://docs.kernel.org/arch/arm/mem_alignment.html

## Verified ARMv7-A/R behavior

ARMv7 requires the newer alignment model. SCTLR.U is Read-As-One (unaligned support is implemented); SCTLR.A controls strict alignment checking. With SCTLR.A clear, selected ordinary load/store instructions can perform unaligned accesses to Normal memory. With SCTLR.A set, those accesses generate an Alignment fault.

| Instruction family | SCTLR.A = 0 | SCTLR.A = 1 |
|---|---|---|
| LDRB / STRB and byte variants | No alignment check | No alignment check |
| LDRH / STRH, LDRSH | Unaligned access permitted | Alignment fault |
| LDR / STR, including ordinary single-word transfers | Unaligned access permitted | Alignment fault |
| LDREX / STREX and exclusive variants | Alignment fault | Alignment fault |
| LDRD / STRD, LDREXD / STREXD | Alignment fault | Alignment fault |
| LDM / STM, PUSH / POP forms subject to the architecture table | Alignment fault | Alignment fault |
| SWP and coprocessor / VFP families listed by the ARM ARM | Alignment fault | Alignment fault |

The table is instruction-family-specific. It is incorrect to generalize "ARMv7 supports unaligned access" to every instruction that transfers multiple bytes.

### Memory-type restriction

The permissive cases apply to Normal memory. An unaligned access to Device or Strongly-ordered memory is not equivalent: the architecture describes such cases as UNPREDICTABLE in applicable non-virtualized configurations, and as an Alignment fault in configurations with the Virtualization Extensions. A single instruction spanning bytes with differing memory type or shareability attributes is also constrained / UNPREDICTABLE by the architecture.

### Important version distinction

Do not copy pre-ARMv6 legacy alignment behavior into an ARMv7 guest. The ARMv7 manual explicitly distinguishes the older model (including legacy address alignment behavior when older control-bit combinations applied) from the ARMv7 model. The old word-load rotate behavior must not be assumed for this ARMv7 target.

## Audit of current repository implementation

The current checked guest memory bus deliberately rejects misaligned 16-bit and 32-bit accesses in:
- `runtime/guest/re_guest_memory_bus.c`: `fetch16/fetch32`, `read16/read32`, and `write16/write32`.
- `runtime/guest/re_guest_cpu_memory.c`: maps the bus range/alignment status to `RE_CPU_E_ALIGNMENT`.
- `runtime/guest/re_guest_cpu_transfer.c`: the isolated Stage 119K/L A32 helper uses the strict facade for word transfers.

The current guest memory region table records address, length, backing pointer, and READ/WRITE/EXEC permissions. It does not record Normal/Device/Strongly-ordered memory type. The CPU model also does not yet expose an emulated SCTLR.A setting to the new checked data-memory path.

Consequently, simply deleting the alignment checks would be unsafe: the bus cannot currently determine whether an access is to Normal memory, and the checked CPU path cannot select strict versus permissive behavior from guest control state.

## Decisions

1. Preserve the current strict behavior as the default and do not change instruction-fetch alignment.
2. Keep byte accesses naturally unaligned; they have no alignment requirement.
3. Implement permissive unaligned semantics only for explicitly supported instruction families and only when the guest configuration allows it.
4. Do not permit unaligned exclusive, doubleword, multi-register, SWP, VFP, or coprocessor accesses through the ordinary scalar helper.
5. Before enabling permissive accesses, add an explicit guest memory attribute (at minimum Normal vs Device/Strongly-ordered) to mapped regions, with a conservative default that is not permissive.
6. Add an explicit guest alignment-control state corresponding to SCTLR.A. Until guest system-register emulation supplies it, the checked helper must remain strict.
7. For permitted ARMv7 Normal-memory LDR/STR and halfword forms, implement the architectural byte-addressed result, not legacy pre-ARMv6 word-rotation behavior. Validate little-endian behavior against the selected guest endianness model.
8. Ensure a multi-byte operation resolves the entire byte range within one compatible mapped region before any store commits. Failed stores must remain all-or-nothing in the emulator's host backing memory.
9. Keep strict alignment for ARM instruction fetch (A32 word-aligned; T32 halfword-aligned). Data alignment rules do not relax instruction alignment.

## Required follow-up work

### Stage 119N — memory attributes and alignment policy

- Extend guest memory mapping with explicit memory attributes while preserving source compatibility or providing a versioned API.
- Define conservative defaults for existing map callers.
- Add a CPU-visible alignment policy object / state with strict mode as the default.
- Add tests proving that Device / unknown memory remains strict.
- Add tests for cross-region accesses and permission faults; no partial write may occur.

### Stage 119O — scalar transfer semantics

- Add isolated checked helpers for A32 LDR/STR word and LDRH/STRH/LDRSH only after the policy layer exists.
- Compare aligned and unaligned cases against an independent reference model.
- Test positive and negative immediate offsets, boundary crossings, little-endian byte order, destination preservation on failed loads, and no mutation on failed stores.
- Keep production decoder migration opt-in until differential coverage is broad enough.

### Later CPU integration gates

- Research the exact guest ARM architecture profile and SCTLR initialization from the original runtime / target ABI evidence.
- Add system-control-register state only when the guest execution model can justify its reset and write behavior.
- Do not activate instructions that remain unsupported or whose fault/writeback semantics are not covered.
- Keep original game boot and physical-device validation as separate milestones.

## Validation and status

This is a documentation-only research stage. No source implementation or runtime behavior is changed here. Host tests from Stage 119L remain the last executable validation baseline. Stage 119M is complete as an architecture decision record; Stage 119N is blocked on implementing the memory-attribute and alignment-policy layer.

No claim is made that the original game has booted or that unaligned data access is implemented.
