# Stage 121A Report — Integrated ELF32 ARM Loader

Status: implementation added; The first host run exposed a test-fixture ordering issue: the W+X case used an undersized buffer, so the loader correctly rejected the buffer before reaching W+X validation. The fixture now provides a valid buffer for the W+X assertion and reduces capacity only for the separate short-buffer assertion. A fresh CI run on the corrected commit is required; the previous run's retry used the old commit and is not evidence for this correction.

The new native loader coordinator connects existing ELF32 image and dynamic
parsers, guest map transaction, segment materializer, relocation engine, and
permission finalizer. It validates load bias, PT_LOAD overlap, caller buffer
capacity, ELF segment flags, and executable entry metadata before starting.
It materializes file bytes and BSS, applies caller-prepared relocation batches,
then finalizes guest permissions. Post-map failures remove mappings created
by the operation.

This is deliberately not described as a complete dynamic linker. Dependency
selection/loading, relocation decoding, external symbol resolution, runtime
constructors, and guest CPU start remain caller responsibilities.

See docs/STAGE121A_ELF32_LOAD_INTEGRATION.md. CI validation pending.
