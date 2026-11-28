# Stage 125A.5 — Guest module preparation session

## Purpose

Connect the already-tested manifest parser, Android-compatible materializer callbacks, ELF32 ARM identity verifier and dependency preflight into one transactional lifecycle operation.

## API

- \`runtime/guest/re_guest_module_session.h\`
- \`runtime/guest/re_guest_module_session.c\`
- \`re_guest_module_session_prepare(...)\`
- \`re_guest_module_session_release(...)\`

## Prepare sequence

1. Parse the bounded v1 module manifest.
2. Materialize every declared asset through caller-supplied source callbacks.
3. Validate every materialized ELF32 ARM identity and manifest size limit.
4. Extract dependency names and build the root-reachable dependency graph.
5. Publish a READY session only after every step succeeds.

On any failure, temporary materialized allocations are released and the caller's output session remains unchanged. Successful sessions own their materialized buffers until release. The caller's source callback context must remain valid for the duration of prepare; materialized buffers are independent after success.

## Regression coverage

A host test uses a synthetic ARM ELF32 shared object and a callback source that deliberately returns short reads. It verifies end-to-end preparation, identity and graph completion, stream closure, release semantics, and manifest-failure rollback. No proprietary game payload is included.

## Explicit boundary

The session is a preparation coordinator, not the ELF loader itself. It does not invoke \`re_guest_elf32_load_pipeline\`, map guest memory, resolve guest imports, apply relocations, execute constructors, initialize the CPU, or run the game entry point. Those remain later lifecycle steps requiring real payload inventory, loader scratch buffers, provider policy and ABI validation.

## Validation

Stage 125A.4 passed Host Source Checks and Android ARM64 native build on commit \`4130a6a54d77bfeeed5c20aa90b7530baa74e119\`. Stage 125A.5 CI is pending on the latest branch head.
