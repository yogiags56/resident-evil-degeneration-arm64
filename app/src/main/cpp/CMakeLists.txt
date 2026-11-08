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
    find_package(LibLZMA REQUIRED)
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
