cmake_minimum_required(VERSION 3.22.1)
project(re_asset_probe C)

add_library(re_asset_probe SHARED
    native_probe.c
    re_file_adapter.c
    re_android_asset_backend.c
    re_android_jni_assets.c
    re_s3e_file_read_contract.c
    re_s3e_file_api_audit.c
    re_s3e_file_bridge.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/archive/re_dtrz_vfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/archive/re_dtrz_file_backend.c
    # Stage 122A: ARM/Thumb execution over permission-checked multi-region guest memory.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_cpu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_cpu_checked.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_cpu_memory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_cpu_scalar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_cpu_arm_scalar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_alignment.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_cpu_transfer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_memory.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_map_transaction.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf_materialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_image.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_xe3u_image.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_xe3u_reloc_plan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_xe3u_import_lookup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_xe3u_import_plan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_dynamic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_link.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_reloc_plan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_linked_loader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_module_graph.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_batch_loader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_provider_catalog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_pipeline.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_init_exec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_boot.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_elf32_loader.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_permission_finalize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_relocation_apply.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_symbol_resolve.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_memory_bus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_cpu_bus_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_memory_view.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_imports.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_dl_bridge.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_dl_provider_catalog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_dl_elf32_adapter.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_dl_provider_bridge.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_format.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_libc_format_provider.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_log_provider.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_zlib_provider.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_heap.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_libc_alloc_provider.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_libc_string_provider.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_cxx_heap_provider.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_eabi_provider.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/s3e_aapcs32.c)

option(RE_ENABLE_LIBLZMA "Enable LZMA-alone DTRZ decoding through liblzma" OFF)
if(RE_ENABLE_LIBLZMA)
    if(NOT LibLZMA_ROOT)
        message(FATAL_ERROR "LibLZMA_ROOT must point to the Android ARM64 liblzma install prefix")
    endif()
    file(GLOB_RECURSE _liblzma_headers
        "${LibLZMA_ROOT}/lzma.h"
        "${LibLZMA_ROOT}/include/lzma.h"
        "${LibLZMA_ROOT}/include/*/lzma.h")
    file(GLOB_RECURSE _liblzma_archives
        "${LibLZMA_ROOT}/liblzma.a"
        "${LibLZMA_ROOT}/lib/liblzma.a"
        "${LibLZMA_ROOT}/lib64/liblzma.a"
        "${LibLZMA_ROOT}/lib/*/liblzma.a"
        "${LibLZMA_ROOT}/lib64/*/liblzma.a")
    list(LENGTH _liblzma_headers _header_count)
    list(LENGTH _liblzma_archives _archive_count)
    if(_header_count EQUAL 0 OR _archive_count EQUAL 0)
        message(FATAL_ERROR "Android liblzma files not found under '${LibLZMA_ROOT}' (headers=${_header_count}, archives=${_archive_count})")
    endif()
    list(GET _liblzma_headers 0 LIBLZMA_INCLUDE_HEADER)
    list(GET _liblzma_archives 0 LIBLZMA_LIBRARY)
    get_filename_component(LIBLZMA_INCLUDE_DIR "${LIBLZMA_INCLUDE_HEADER}" DIRECTORY)
    add_library(LibLZMA::LibLZMA UNKNOWN IMPORTED)
    set_target_properties(LibLZMA::LibLZMA PROPERTIES
        IMPORTED_LOCATION "${LIBLZMA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBLZMA_INCLUDE_DIR}")
    target_sources(re_asset_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/archive/re_dtrz_lzma.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest/re_guest_xe3u_decode.c)
    target_link_libraries(re_asset_probe LibLZMA::LibLZMA)
    target_compile_definitions(re_asset_probe PRIVATE RE_HAVE_LIBLZMA=1)
endif()

find_library(log-lib log)
find_library(android-lib android)
find_library(z-lib z)
target_include_directories(re_asset_probe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/archive
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest)
target_compile_options(re_asset_probe PRIVATE -Wall -Wextra -Werror)
target_link_libraries(re_asset_probe ${android-lib} ${log-lib} ${z-lib})
