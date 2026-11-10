#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
python3 -m unittest discover -s tests -v
bash ./tools/compile_host_mock.sh
mkdir -p build/host-tests
CC="${CC:-clang}"
CFLAGS=(-std=c11 -Wall -Wextra -Werror -I runtime/guest -I app/src/main/cpp -I.)
"$CC" "${CFLAGS[@]}" tests/test_s3e_file_read_contract.c \
  app/src/main/cpp/re_s3e_file_read_contract.c \
  -o build/host-tests/test_s3e_file_read_contract
"$CC" "${CFLAGS[@]}" tests/test_re_s3e_file_api_audit.c \
  app/src/main/cpp/re_s3e_file_api_audit.c \
  -o build/host-tests/test_re_s3e_file_api_audit
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_cpu
"$CC" "${CFLAGS[@]}" tests/test_re_guest_memory.c runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_view.c \
  -o build/host-tests/test_re_guest_memory
"$CC" "${CFLAGS[@]}" tests/test_re_guest_map_transaction.c \
  runtime/guest/re_guest_map_transaction.c runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_map_transaction
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf_materialize.c \
  runtime/guest/re_guest_elf_materialize.c runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_elf_materialize
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_image.c \
  runtime/guest/re_guest_elf32_image.c \
  -o build/host-tests/test_re_guest_elf32_image
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_dynamic.c \
  runtime/guest/re_guest_elf32_dynamic.c runtime/guest/re_guest_elf32_image.c \
  -o build/host-tests/test_re_guest_elf32_dynamic
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_link.c \
  runtime/guest/re_guest_elf32_link.c runtime/guest/re_guest_elf32_dynamic.c \
  runtime/guest/re_guest_elf32_image.c \
  -o build/host-tests/test_re_guest_elf32_link
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_reloc_plan.c \
  runtime/guest/re_guest_elf32_reloc_plan.c runtime/guest/re_guest_symbol_resolve.c \
  -o build/host-tests/test_re_guest_elf32_reloc_plan
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_module_graph.c \
  runtime/guest/re_guest_elf32_module_graph.c \
  -o build/host-tests/test_re_guest_elf32_module_graph
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_provider_catalog.c \
  runtime/guest/re_guest_elf32_provider_catalog.c \
  -o build/host-tests/test_re_guest_elf32_provider_catalog
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_batch_loader.c \
  runtime/guest/re_guest_elf32_batch_loader.c runtime/guest/re_guest_elf32_loader.c \
  runtime/guest/re_guest_elf32_dynamic.c runtime/guest/re_guest_elf32_image.c \
  runtime/guest/re_guest_map_transaction.c runtime/guest/re_guest_elf_materialize.c \
  runtime/guest/re_guest_permission_finalize.c runtime/guest/re_guest_relocation_apply.c \
  runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_elf32_batch_loader
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_linked_loader.c \
  runtime/guest/re_guest_elf32_linked_loader.c \
  runtime/guest/re_guest_elf32_reloc_plan.c runtime/guest/re_guest_elf32_link.c \
  runtime/guest/re_guest_elf32_loader.c runtime/guest/re_guest_elf32_dynamic.c \
  runtime/guest/re_guest_elf32_image.c runtime/guest/re_guest_map_transaction.c \
  runtime/guest/re_guest_elf_materialize.c runtime/guest/re_guest_permission_finalize.c \
  runtime/guest/re_guest_relocation_apply.c runtime/guest/re_guest_symbol_resolve.c \
  runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_elf32_linked_loader
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_loader.c \
  runtime/guest/re_guest_elf32_loader.c runtime/guest/re_guest_elf32_dynamic.c \
  runtime/guest/re_guest_elf32_image.c runtime/guest/re_guest_map_transaction.c \
  runtime/guest/re_guest_elf_materialize.c runtime/guest/re_guest_permission_finalize.c \
  runtime/guest/re_guest_relocation_apply.c runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_elf32_loader
"$CC" "${CFLAGS[@]}" tests/test_re_guest_permission_finalize.c \
  runtime/guest/re_guest_permission_finalize.c runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_permission_finalize
"$CC" "${CFLAGS[@]}" tests/test_re_guest_relocation_apply.c \
  runtime/guest/re_guest_relocation_apply.c runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_relocation_apply
"$CC" "${CFLAGS[@]}" tests/test_re_guest_symbol_resolve.c \
  runtime/guest/re_guest_symbol_resolve.c \
  -o build/host-tests/test_re_guest_symbol_resolve
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf_pipeline.c \
  runtime/guest/re_guest_map_transaction.c \
  runtime/guest/re_guest_elf_materialize.c \
  runtime/guest/re_guest_permission_finalize.c \
  runtime/guest/re_guest_relocation_apply.c \
  runtime/guest/re_guest_symbol_resolve.c \
  runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_elf_pipeline
"$CC" "${CFLAGS[@]}" tests/test_re_guest_memory_bus.c runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_memory_bus
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_bus_adapter.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_cpu_bus_adapter
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_instruction_fetch.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_cpu_instruction_fetch
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_checked_step.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_cpu_checked.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_cpu_checked_step
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_mapped.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_cpu_mapped
"$CC" "${CFLAGS[@]}" tests/test_re_guest_alignment.c \
  runtime/guest/re_guest_alignment.c runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_alignment
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_scalar.c \
  runtime/guest/re_guest_cpu_scalar.c runtime/guest/re_guest_alignment.c \
  runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_cpu_scalar
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_arm_scalar.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_cpu_arm_scalar.c \
  runtime/guest/re_guest_cpu_scalar.c \
  runtime/guest/re_guest_alignment.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_cpu_arm_scalar
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_arm_scalar_differential.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_cpu_arm_scalar.c \
  runtime/guest/re_guest_cpu_scalar.c runtime/guest/re_guest_alignment.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_cpu_arm_scalar_differential
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_arm_scalar_faults.c \
  runtime/guest/re_guest_cpu_arm_scalar.c runtime/guest/re_guest_cpu_scalar.c \
  runtime/guest/re_guest_alignment.c runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_cpu_arm_scalar_faults
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_memory.c \
  runtime/guest/re_guest_cpu_memory.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_cpu_memory
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cpu_transfer.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_cpu_transfer.c \
  runtime/guest/re_guest_cpu_memory.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_cpu_transfer
"$CC" "${CFLAGS[@]}" tests/test_re_guest_imports.c \
  runtime/guest/re_guest_imports.c runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_imports
"$CC" "${CFLAGS[@]}" tests/test_re_guest_runtime.c \
  runtime/guest/re_guest_runtime.c runtime/guest/re_guest_imports.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_runtime
"$CC" "${CFLAGS[@]}" tests/test_re_guest_runtime_mapped.c \
  runtime/guest/re_guest_runtime.c runtime/guest/re_guest_imports.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_runtime_mapped
"$CC" "${CFLAGS[@]}" tests/test_re_guest_gl_imports.c \
  runtime/guest/re_guest_gl_imports.c runtime/guest/re_guest_runtime.c \
  runtime/guest/re_guest_imports.c runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_gl_imports
"$CC" "${CFLAGS[@]}" -I runtime/archive tests/test_re_dtrz_vfs.c \
  runtime/archive/re_dtrz_vfs.c -o build/host-tests/test_re_dtrz_vfs

if pkg-config --exists liblzma; then
  LZMA_CFLAGS=( $(pkg-config --cflags liblzma) )
  LZMA_LIBS=( $(pkg-config --libs liblzma) )
  "$CC" "${CFLAGS[@]}" "${LZMA_CFLAGS[@]}" -I runtime/archive \
    tests/test_re_dtrz_lzma.c runtime/archive/re_dtrz_lzma.c \
    "${LZMA_LIBS[@]}" -o build/host-tests/test_re_dtrz_lzma
  "$CC" "${CFLAGS[@]}" "${LZMA_CFLAGS[@]}" -I runtime/archive \
    tools/dtrz_native_verify.c runtime/archive/re_dtrz_vfs.c \
    runtime/archive/re_dtrz_lzma.c "${LZMA_LIBS[@]}" \
    -o build/host-tests/dtrz_native_verify
else
  echo "SKIP liblzma integration: install liblzma development package"
fi

"$CC" "${CFLAGS[@]}" -I app/src/main/cpp -I runtime/guest \
  tests/test_re_file_adapter.c app/src/main/cpp/re_file_adapter.c \
  -o build/host-tests/test_re_file_adapter
"$CC" "${CFLAGS[@]}" -I runtime/archive -I app/src/main/cpp -I runtime/guest \
  tests/test_re_dtrz_file_backend.c runtime/archive/re_dtrz_vfs.c \
  runtime/archive/re_dtrz_file_backend.c app/src/main/cpp/re_file_adapter.c \
  -o build/host-tests/test_re_dtrz_file_backend

"$CC" "${CFLAGS[@]}" -I runtime/guest -I app/src/main/cpp \
  tests/test_re_s3e_file_import_dispatch.c \
  runtime/guest/re_guest_imports.c runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c runtime/guest/re_guest_cpu_bus_adapter.c \
  app/src/main/cpp/re_s3e_file_api_audit.c \
  app/src/main/cpp/re_s3e_file_read_contract.c \
  -o build/host-tests/test_re_s3e_file_import_dispatch
"$CC" "${CFLAGS[@]}" -I runtime/guest -I app/src/main/cpp \
  tests/test_re_s3e_file_bridge.c app/src/main/cpp/re_s3e_file_bridge.c \
  app/src/main/cpp/re_file_adapter.c app/src/main/cpp/re_s3e_file_api_audit.c \
  app/src/main/cpp/re_s3e_file_read_contract.c \
  -o build/host-tests/test_re_s3e_file_bridge
"$CC" "${CFLAGS[@]}" -I runtime/guest -I app/src/main/cpp -I runtime/archive \
  tests/test_re_s3e_file_bridge_dispatch_dtrz.c \
  runtime/guest/re_guest_imports.c runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  runtime/archive/re_dtrz_vfs.c runtime/archive/re_dtrz_file_backend.c \
  app/src/main/cpp/re_s3e_file_bridge.c app/src/main/cpp/re_file_adapter.c \
  app/src/main/cpp/re_s3e_file_api_audit.c app/src/main/cpp/re_s3e_file_read_contract.c \
  -o build/host-tests/test_re_s3e_file_bridge_dispatch_dtrz

for test in build/host-tests/test_*; do
  echo "RUN $test"
  "$test"
done
