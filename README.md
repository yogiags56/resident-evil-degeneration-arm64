## Current engineering milestone — Stage 119T

Stage 119T adds a read-only ELF/ARM ABI metadata auditor with synthetic fixtures. It inspects locally supplied ELF32/ELF64 files and selected AEABI .ARM.attributes; it does not execute or modify the binary. The original game executable is not stored in this repository, so game-specific CPU attributes remain unknown.

    python3 tools/guest_arm_elf_audit.py /path/to/guest.elf --json /tmp/guest-elf-report.json

See docs/STAGE119T_GUEST_ELF_ARM_ABI_AUDIT.md, docs/STATUS.md, and ROADMAP.md. Host CI is pending. No game boot or playable build is claimed.
