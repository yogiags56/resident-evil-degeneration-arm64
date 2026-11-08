# Contributing

Thanks for helping with the ARM64 compatibility research.

## Before submitting
- Explain the exact runtime or binary behavior being addressed.
- Link the evidence (callsite, disassembly range, API documentation, or reproducible test).
- Mark claims as `observed`, `inferred`, or `unknown`.
- Add a regression test for every bug fix where practical.
- Run `python3 -m unittest discover -s tests -v`.
- Run `./tools/compile_host_mock.sh`.

## C/C++ conventions
- C11 for portable runtime code.
- Fixed-width integer types for guest registers and addresses.
- Never cast a guest 32-bit address directly to a host pointer.
- Check bounds and integer overflow before memory access.
- Keep Android-specific code behind the backend/JNI boundary.
- Avoid enabling guest imports based only on symbol names.

## Asset policy
Do not commit the original game APK or extracted game assets. Use local ignored asset directories and document hashes/metadata when necessary without uploading proprietary payloads.
