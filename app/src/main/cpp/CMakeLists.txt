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
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/archive/re_dtrz_file_backend.c)

# liblzma is an explicit dependency. Supply a CMake package/toolchain build
# for Android and enable this option only when that dependency is available.
option(RE_ENABLE_LIBLZMA "Enable LZMA-alone DTRZ decoding through liblzma" OFF)
if(RE_ENABLE_LIBLZMA)
    # The Android cross-build installs liblzma into a private prefix. Resolve
    # the actual installed files recursively: XZ Utils may use lib or lib64,
    # and Android/NDK CMake must never fall back to host libraries.
    if(NOT LibLZMA_ROOT)
        message(FATAL_ERROR "LibLZMA_ROOT must point to the Android ARM64 liblzma install prefix")
    endif()
    file(GLOB_RECURSE _liblzma_headers
        "${LibLZMA_ROOT}/lzma.h"
        "${LibLZMA_ROOT}/include/lzma.h"
        "${LibLZMA_ROOT}/include/*/lzma.h")
    file(GLOB_RECURSE _liblzma_archives
        "${LibLZMA_ROOT}/liblzma.a"
        "${LibLZMA_ROOT}/lib/*/liblzma.a"
        "${LibLZMA_ROOT}/lib64/*/liblzma.a"
        "${LibLZMA_ROOT}/lib/liblzma.a"
        "${LibLZMA_ROOT}/lib64/liblzma.a")
    list(LENGTH _liblzma_headers _header_count)
    list(LENGTH _liblzma_archives _archive_count)
    if(_header_count EQUAL 0 OR _archive_count EQUAL 0)
        message(FATAL_ERROR
            "Android liblzma files not found under '${LibLZMA_ROOT}' "
            "(headers=${_header_count}, archives=${_archive_count})")
    endif()
    list(GET _liblzma_headers 0 LIBLZMA_INCLUDE_HEADER)
    list(GET _liblzma_archives 0 LIBLZMA_LIBRARY)
    get_filename_component(LIBLZMA_INCLUDE_DIR "${LIBLZMA_INCLUDE_HEADER}" DIRECTORY)
    add_library(LibLZMA::LibLZMA UNKNOWN IMPORTED)
    set_target_properties(LibLZMA::LibLZMA PROPERTIES
        IMPORTED_LOCATION "${LIBLZMA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBLZMA_INCLUDE_DIR}")
    target_sources(re_asset_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/archive/re_dtrz_lzma.c)
    target_link_libraries(re_asset_probe LibLZMA::LibLZMA)
    target_compile_definitions(re_asset_probe PRIVATE RE_HAVE_LIBLZMA=1)
endif()
    find_path(LIBLZMA_INCLUDE_DIR
        NAMES lzma.h
        PATHS "${LibLZMA_ROOT}/include"
        NO_DEFAULT_PATH
        REQUIRED)
    find_library(LIBLZMA_LIBRARY
        NAMES lzma
        PATHS "${LibLZMA_ROOT}/lib" "${LibLZMA_ROOT}/lib64"
        NO_DEFAULT_PATH
        REQUIRED)
    add_library(LibLZMA::LibLZMA UNKNOWN IMPORTED)
    set_target_properties(LibLZMA::LibLZMA PROPERTIES
        IMPORTED_LOCATION "${LIBLZMA_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBLZMA_INCLUDE_DIR}")
    target_sources(re_asset_probe PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/archive/re_dtrz_lzma.c)
    target_link_libraries(re_asset_probe LibLZMA::LibLZMA)
    target_compile_definitions(re_asset_probe PRIVATE RE_HAVE_LIBLZMA=1)
endif()
find_library(log-lib log)
find_library(android-lib android)
target_include_directories(re_asset_probe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/archive
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../runtime/guest)
target_compile_options(re_asset_probe PRIVATE -Wall -Wextra -Werror)
target_link_libraries(re_asset_probe ${android-lib} ${log-lib})
