# Stage 119S — Guest ARM architecture-profile evidence audit

## Purpose

Audit the evidence available in this repository before adding emulated system-control state or changing alignment defaults. This stage deliberately avoids assigning an ARM profile that the available artifacts do not prove.

## Evidence currently present

### Confirmed by repository analysis

1. The guest image has been analyzed as ARM32/Thumb code and its imports use the AAPCS32 core-register convention. The Stage 81 ABI report documents arguments in r0-r3 and stack argument words at entry SP.
2. The guest CPU model is a 32-bit core-register machine with r0-r15, CPSR condition flags, ARM/Thumb state, and Thumb IT state.
3. The interpreter is partial. It does not currently emulate a complete privileged ARM system-control environment.
4. The native application target is Android ARM64. That describes the host process ABI, not the architecture profile of the emulated guest.
5. The Stage 119M alignment report cites the ARMv7-A/R architecture manual to describe conditional alignment rules. Citing that manual is not evidence that this particular guest is ARMv7-A or ARMv7-R.
6. The current guest scalar-memory helper assembles little-endian values. This is an implementation convention that still needs confirmation against the original guest binary/runtime evidence.

## Profile attributes not established

The repository does not currently contain enough primary guest-binary evidence to establish:

- exact ARM architecture revision (for example ARMv6 versus ARMv7);
- A-profile versus R-profile;
- CPU implementer/part number or specific core;
- Thumb-2 requirement and exact optional instruction extensions;
- VFP version, floating-point ABI, or NEON/Advanced SIMD requirements;
- guest endianness control and reset state;
- privileged execution mode and exception model;
- MMU presence/configuration and page-table regime;
- SCTLR reset value, writable bits, or the initial SCTLR.A value;
- guest memory-region type and shareability attributes.

Do not infer any of these from the Android ARM64 host build or from the broad ARMv7-A/R alignment reference.

## Artifact gap

The original game executable and proprietary package are not distributed in this repository. Existing DTRZ research covers archive metadata and payload windows, but a decoded original executable plus its ELF header/ARM attributes or equivalent platform metadata is not currently available as a tracked, reproducible input for this audit.

A future binary evidence pass should capture, where present:

- ELF class, endianness, machine, ABI flags, and program headers;
- .ARM.attributes values such as Tag_CPU_arch, Tag_CPU_arch_profile, Tag_THUMB_ISA_use, Tag_FP_arch, and Tag_Advanced_SIMD_arch;
- relocation types and symbol/import metadata;
- an instruction inventory for A32/T32 and optional extensions;
- original runtime/SDK startup code that initializes control registers and privilege state.

Metadata alone may not prove runtime SCTLR state; startup code or authoritative platform documentation may still be needed.

## Decisions

1. Do not add guessed SCTLR reset values or pretend the guest is a specific ARM core.
2. Keep the current strict-by-default alignment policy as a conservative emulator policy, not as a claim about the original guest's reset state.
3. Do not expose a writable SCTLR.A state to production instruction execution until the guest profile and reset/write semantics are evidenced.
4. Keep ARMv7-A/R alignment rules conditional and instruction-specific; do not generalize permissive unaligned access to all instructions or memory types.
5. Keep the Stage 119P helper isolated from re_guest_cpu_step.
6. Treat host ARM64 ELF verification as host-build evidence only.

## Next evidence stage

Stage 119T should prepare a reproducible guest-binary metadata and instruction-inventory report when the original executable or a legally distributable sample is available. Until then, the profile remains intentionally unresolved.

## Validation and status

Documentation-only audit. Stage 119P host tests and Android ARM64 build passed. Stage 119Q host differential tests (512 cases) and Android ARM64 build passed. Stage 119R host CI passed; its Android ARM64 build is still running.

No game boot, physical-device execution, guest system-control emulation, or exact ARM profile is claimed.
