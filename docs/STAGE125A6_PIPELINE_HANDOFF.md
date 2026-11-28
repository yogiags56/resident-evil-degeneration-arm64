# Stage 125A.6 — Manifest-bound ELF32 pipeline handoff

## Purpose

Connect a READY module preparation session to the existing ELF32 load pipeline without allowing the pipeline inventory to silently diverge from the validated manifest and materialized asset set.

## API

- `runtime/guest/re_guest_module_session_load.h`
- `runtime/guest/re_guest_module_session_load.c`
- `re_guest_module_session_load(...)`

## Handoff contract

Before calling `re_guest_elf32_load_pipeline`, the adapter requires:
- Session state is READY.
- Pipeline module count equals the parsed manifest count.
- Root name exactly matches the manifest root entry.
- Each pipeline module name matches the manifest entry at the same index.
- Each pipeline ELF pointer and byte size exactly match the corresponding session-owned materialized buffer.

The caller remains responsible for guest memory, load biases, per-segment host buffers, symbol tables, decoded/prepared relocation scratch, and provider scratch. The existing pipeline performs metadata parsing, provider collection, relocation preparation, and dependency-first batch mapping/loading.

On success the session transitions to LOADED and the pipeline result is published. On failure the session remains READY; borrowed scratch buffers must be discarded or reinitialized before retry. The batch loader restores the guest mapping table on a failed batch.

## Safety boundary

This stage wires an existing loading pipeline; it does not claim original game payload compatibility, Bionic emulation, constructor completion, CPU handoff, or gameplay. Real manifest inventory and payload-specific scratch sizing still need validation. No game boot is claimed.

## Validation

Stage 125A.5 CI exposed a manifest status enum spelling error and an unused test callback parameter. Both were corrected in commits `33f07a26a2a84a5426ce6595cfa094788f2ff02d` and `0a7727b30bdc0036d2f82bb82b16bc6c81231672`; revalidation is running. Stage 125A.6 is pending CI on the latest branch head.
