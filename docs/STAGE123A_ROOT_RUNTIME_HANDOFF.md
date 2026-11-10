# Stage 123A — Root executable runtime handoff

## Implemented
- Pipeline result now records the root module's inventory index, independent of dependency-first load order.
- Added `re_guest_elf32_boot.h/.c`.
- Finds the root module's corresponding batch-load result by searching the dependency-first graph order.
- Cross-checks the loaded entry point against the parsed ELF entry plus that module's load bias.
- Verifies the root entry address is mapped executable with the expected ARM/Thumb instruction width.
- Verifies a caller-provided stack has at least 256 readable/writable bytes below an 8-byte-aligned stack pointer.
- Initializes the mapped AArch32 guest runtime at the root executable entry and installs the requested initial SP.
- Returns explicit boot metadata: root inventory index, guest entry pointer, stack pointer, and Thumb state.
- Does not execute the root entry; caller can now run it through the bounded runtime API after constructors have completed.
- Added a regression where root is inventory index 0 but loaded second (dependency-first batch order index 1); validates the handoff selects the root entry correctly.
- Registered boot handoff source in Android native CMake and host test suite.

## Remaining boot requirements
- The Android asset layer must discover and provide the real executable and all DT_NEEDED module bytes, load biases, and per-segment buffers.
- S3E import inventory/bindings, TLS, process globals, Bionic behavior, and complete ARM instruction coverage are still required for a real game boot.
- The root runtime handoff is synthetic-test validated only; no proprietary game entry point has been executed yet.

## Validation
Host CI run 42867835687 completed successfully for commit `749de137a8918d32df56e8de8b0004cf75099539`. Logs explicitly show:
- Guest ELF32 root boot handoff tests: PASS
- Guest ELF32 constructor sequence tests: PASS
- Guest ELF32 bounded constructor execution tests: PASS

Android ARM64 build and actual game boot remain separate validation gates.
