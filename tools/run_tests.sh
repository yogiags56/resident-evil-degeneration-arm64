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
"$CC" "${CFLAGS[@]}" tests/test_re_guest_xe3u_image.c \
  runtime/guest/re_guest_xe3u_image.c \
  -o build/host-tests/test_re_guest_xe3u_image
"$CC" "${CFLAGS[@]}" tests/test_re_guest_xe3u_reloc_plan.c \
  runtime/guest/re_guest_xe3u_reloc_plan.c \
  -o build/host-tests/test_re_guest_xe3u_reloc_plan
"$CC" "${CFLAGS[@]}" tests/test_re_guest_xe3u_import_lookup.c \
  runtime/guest/re_guest_xe3u_import_lookup.c \
  -o build/host-tests/test_re_guest_xe3u_import_lookup
"$CC" "${CFLAGS[@]}" tests/test_re_guest_xe3u_import_plan.c \
  runtime/guest/re_guest_xe3u_import_plan.c runtime/guest/re_guest_imports.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_xe3u_import_plan
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
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_cross_module_link.c \
  runtime/guest/re_guest_elf32_provider_catalog.c runtime/guest/re_guest_elf32_reloc_plan.c \
  runtime/guest/re_guest_symbol_resolve.c \
  -o build/host-tests/test_re_guest_elf32_cross_module_link
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_boot.c \
  runtime/guest/re_guest_elf32_boot.c runtime/guest/re_guest_runtime.c \
  runtime/guest/re_guest_imports.c runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_elf32_boot
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_init_exec.c \
  runtime/guest/re_guest_elf32_init_exec.c runtime/guest/re_guest_runtime.c \
  runtime/guest/re_guest_imports.c runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_elf32_init_exec
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_init.c \
  runtime/guest/re_guest_elf32_init.c runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_elf32_init
"$CC" "${CFLAGS[@]}" tests/test_re_guest_elf32_pipeline.c \
  runtime/guest/re_guest_elf32_pipeline.c runtime/guest/re_guest_elf32_provider_catalog.c \
  runtime/guest/re_guest_elf32_module_graph.c runtime/guest/re_guest_elf32_reloc_plan.c \
  runtime/guest/re_guest_elf32_link.c runtime/guest/re_guest_elf32_dynamic.c \
  runtime/guest/re_guest_elf32_image.c runtime/guest/re_guest_elf32_batch_loader.c \
  runtime/guest/re_guest_elf32_loader.c runtime/guest/re_guest_symbol_resolve.c \
  runtime/guest/re_guest_map_transaction.c runtime/guest/re_guest_elf_materialize.c \
  runtime/guest/re_guest_permission_finalize.c runtime/guest/re_guest_relocation_apply.c \
  runtime/guest/re_guest_memory.c \
  -o build/host-tests/test_re_guest_elf32_pipeline
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
"$CC" "${CFLAGS[@]}" tests/test_re_guest_xe3u_import_dispatch.c \
  runtime/guest/re_guest_runtime.c runtime/guest/re_guest_imports.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_xe3u_import_dispatch
"$CC" "${CFLAGS[@]}" tests/test_re_guest_runtime_mapped.c \
  runtime/guest/re_guest_runtime.c runtime/guest/re_guest_imports.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_runtime_mapped
"$CC" "${CFLAGS[@]}" tests/test_re_guest_dl_bridge.c \
  runtime/guest/re_guest_dl_bridge.c runtime/guest/re_guest_runtime.c \
  runtime/guest/re_guest_imports.c runtime/guest/re_guest_symbol_resolve.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_dl_bridge
"$CC" "${CFLAGS[@]}" tests/test_re_guest_dl_provider_catalog.c \
  runtime/guest/re_guest_dl_provider_catalog.c runtime/guest/re_guest_symbol_resolve.c \
  -o build/host-tests/test_re_guest_dl_provider_catalog
"$CC" "${CFLAGS[@]}" tests/test_re_guest_dl_elf32_adapter.c \
  runtime/guest/re_guest_dl_elf32_adapter.c \
  runtime/guest/re_guest_dl_provider_catalog.c \
  runtime/guest/re_guest_elf32_provider_catalog.c \
  runtime/guest/re_guest_symbol_resolve.c \
  -o build/host-tests/test_re_guest_dl_elf32_adapter
"$CC" "${CFLAGS[@]}" tests/test_re_guest_dl_provider_bridge.c \
  runtime/guest/re_guest_dl_provider_bridge.c \
  runtime/guest/re_guest_dl_bridge.c runtime/guest/re_guest_dl_provider_catalog.c \
  runtime/guest/re_guest_symbol_resolve.c runtime/guest/re_guest_imports.c \
  runtime/guest/re_guest_runtime.c runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_dl_provider_bridge
"$CC" "${CFLAGS[@]}" tests/test_re_guest_log_provider.c \
  runtime/guest/re_guest_log_provider.c runtime/guest/re_guest_format.c \
  runtime/guest/re_guest_imports.c runtime/guest/re_guest_runtime.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_log_provider
"$CC" "${CFLAGS[@]}" tests/test_re_guest_libc_format_provider.c \
  runtime/guest/re_guest_libc_format_provider.c runtime/guest/re_guest_format.c \
  runtime/guest/re_guest_imports.c runtime/guest/re_guest_runtime.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_libc_format_provider
"$CC" "${CFLAGS[@]}" tests/test_re_guest_zlib_provider.c \
  runtime/guest/re_guest_zlib_provider.c runtime/guest/re_guest_imports.c \
  runtime/guest/re_guest_runtime.c runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c runtime/guest/re_guest_cpu_bus_adapter.c \
  -lz -o build/host-tests/test_re_guest_zlib_provider
"$CC" "${CFLAGS[@]}" tests/test_re_guest_cxx_heap_provider.c \
  runtime/guest/re_guest_cxx_heap_provider.c runtime/guest/re_guest_heap.c \
  runtime/guest/re_guest_imports.c runtime/guest/re_guest_runtime.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_cxx_heap_provider
"$CC" "${CFLAGS[@]}" tests/test_re_guest_libc_alloc_provider.c \
  runtime/guest/re_guest_libc_alloc_provider.c \
  runtime/guest/re_guest_cxx_heap_provider.c runtime/guest/re_guest_heap.c \
  runtime/guest/re_guest_imports.c runtime/guest/re_guest_runtime.c \
  runtime/guest/s3e_aapcs32.c runtime/guest/re_guest_cpu.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_libc_alloc_provider
"$CC" "${CFLAGS[@]}" tests/test_re_guest_libc_string_provider.c \
  runtime/guest/re_guest_libc_string_provider.c runtime/guest/re_guest_imports.c \
  runtime/guest/re_guest_runtime.c runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_libc_string_provider
"$CC" "${CFLAGS[@]}" tests/test_re_guest_scan.c \
  runtime/guest/re_guest_scan.c runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_memory.c runtime/guest/re_guest_memory_bus.c \
  -o build/host-tests/test_re_guest_scan
"$CC" "${CFLAGS[@]}" tests/test_re_guest_libc_stdio_provider.c \
  runtime/guest/re_guest_libc_stdio_provider.c runtime/guest/re_guest_format.c \
  runtime/guest/re_guest_scan.c \
  runtime/guest/re_guest_imports.c runtime/guest/re_guest_runtime.c \
  runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_libc_stdio_provider
"$CC" "${CFLAGS[@]}" tests/test_re_guest_eabi_provider.c \
  runtime/guest/re_guest_eabi_provider.c runtime/guest/re_guest_imports.c \
  runtime/guest/re_guest_runtime.c runtime/guest/s3e_aapcs32.c \
  runtime/guest/re_guest_cpu.c runtime/guest/re_guest_memory.c \
  runtime/guest/re_guest_memory_bus.c runtime/guest/re_guest_cpu_bus_adapter.c \
  -o build/host-tests/test_re_guest_eabi_provider
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
    tests/test_re_guest_xe3u_decode.c runtime/guest/re_guest_xe3u_decode.c \
    runtime/guest/re_guest_xe3u_image.c runtime/archive/re_dtrz_lzma.c \
    "${LZMA_LIBS[@]}" -o build/host-tests/test_re_guest_xe3u_decode
  "$CC" "${CFLAGS[@]}" "${LZMA_CFLAGS[@]}" -I runtime/archive \
    tools/xe3u_native_audit.c runtime/guest/re_guest_xe3u_decode.c \
    runtime/guest/re_guest_xe3u_image.c runtime/archive/re_dtrz_lzma.c \
    "${LZMA_LIBS[@]}" -o build/host-tests/xe3u_native_audit
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
