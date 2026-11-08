cmake_minimum_required(VERSION 3.22.1)
project(re_liblzma_host_smoke C)
find_package(LibLZMA REQUIRED)
add_executable(test_re_dtrz_lzma
    "${CMAKE_CURRENT_LIST_DIR}/../test_re_dtrz_lzma.c"
    "${CMAKE_CURRENT_LIST_DIR}/../../runtime/archive/re_dtrz_lzma.c")
target_include_directories(test_re_dtrz_lzma PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../runtime/archive")
target_link_libraries(test_re_dtrz_lzma PRIVATE LibLZMA::LibLZMA)
target_compile_options(test_re_dtrz_lzma PRIVATE -Wall -Wextra -Werror -UNDEBUG)
