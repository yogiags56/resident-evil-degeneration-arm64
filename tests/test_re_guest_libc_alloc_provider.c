#include "re_guest_cxx_heap_provider.h"
#include "re_guest_libc_alloc_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void call_import(REGUESTRuntime *runtime, uint32_t pointer,
                        uint32_t return_address, uint32_t r0, uint32_t r1) {
    runtime->cpu.r[15] = pointer & ~1u;
    runtime->cpu.thumb = 1u;
    runtime->cpu.r[14] = return_address | 1u;
    runtime->cpu.r[13] = 0x1000f000u;
    runtime->cpu.r[0] = r0;
    runtime->cpu.r[1] = r1;
    runtime->cpu.r[2] = 0u;
    runtime->cpu.r[3] = 0u;
    assert(re_guest_runtime_step(runtime) == RE_RUNTIME_OK);
    assert(runtime->cpu.r[15] == return_address);
}

int main(void) {
    static uint8_t guest[0x10000];
    const uint32_t guest_base = 0x10000000u;
    const uint32_t heap_base = guest_base + 0x4000u;
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTCXXHeapProvider cxx;
    REGUESTLIBCAllocProvider libc;
    REGUESTELF32LINKSYMBOL symbols[9] = {0};
    REGUESTSYMBOL cxx_exports[4], libc_exports[4];
    size_t cxx_count = 0u, libc_count = 0u, failed = 0u;
    void *mapped = NULL;

    (void)strcpy(symbols[1].name, "_Znwj");
    (void)strcpy(symbols[2].name, "_Znaj");
    (void)strcpy(symbols[3].name, "_ZdlPv");
    (void)strcpy(symbols[4].name, "_ZdaPv");
    (void)strcpy(symbols[5].name, "malloc");
    (void)strcpy(symbols[6].name, "free");
    (void)strcpy(symbols[7].name, "realloc");
    (void)strcpy(symbols[8].name, "valloc");
    for (size_t i = 1u; i < 9u; ++i) {
        symbols[i].section_index = 0u;
        symbols[i].binding = RE_GUEST_STB_GLOBAL;
        symbols[i].type = 2u;
        symbols[i].visibility = RE_GUEST_STV_DEFAULT;
    }

    assert(re_guest_cxx_heap_provider_init(&cxx, heap_base, 0x8000u) ==
           RE_GUEST_CXX_HEAP_OK);
    assert(re_guest_libc_alloc_provider_init(&libc, &cxx.heap) ==
           RE_GUEST_LIBC_ALLOC_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_cxx_heap_provider_bind_elf_symbols(&cxx, &imports,
        symbols, 9u, cxx_exports, 4u, &cxx_count, &failed) ==
        RE_GUEST_CXX_HEAP_OK);
    assert(cxx_count == 4u && failed == 9u);
    assert(re_guest_libc_alloc_provider_bind_elf_symbols(&libc, &imports,
        symbols, 9u, libc_exports, 4u, &libc_count, &failed) ==
        RE_GUEST_LIBC_ALLOC_OK);
    assert(libc_count == 4u && failed == 9u);
    assert(imports.count == 8u);
    for (size_t i = 0u; i < 4u; ++i)
        assert(strcmp(libc_exports[i].version_library, "libc.so") == 0);

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, guest_base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 guest_base, 1);

    /* libc malloc and C++ delete share exact allocation ownership. */
    call_import(&runtime, libc_exports[0].value, guest_base + 0x20u, 24u, 0u);
    const uint32_t malloc_ptr = runtime.cpu.r[0];
    assert(malloc_ptr == heap_base && (malloc_ptr & 7u) == 0u);
    call_import(&runtime, cxx_exports[2].value, guest_base + 0x40u,
                malloc_ptr, 0u);
    assert(runtime.cpu.r[0] == 0u);

    /* C++ new and libc free also share the same allocation records. */
    call_import(&runtime, cxx_exports[0].value, guest_base + 0x60u, 24u, 0u);
    const uint32_t new_ptr = runtime.cpu.r[0];
    assert(new_ptr == heap_base);
    call_import(&runtime, libc_exports[1].value, guest_base + 0x80u,
                new_ptr, 0u);
    assert(runtime.cpu.r[0] == 0u);

    /* realloc moves to a new guest block and preserves the old byte prefix. */
    call_import(&runtime, libc_exports[0].value, guest_base + 0xa0u, 16u, 0u);
    const uint32_t old_ptr = runtime.cpu.r[0];
    assert(re_guest_memory_resolve(&memory, old_ptr, 16u,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE, &mapped) == RE_GUEST_MEM_OK);
    memset(mapped, 0x6d, 16u);
    call_import(&runtime, libc_exports[2].value, guest_base + 0xc0u,
                old_ptr, 64u);
    const uint32_t grown_ptr = runtime.cpu.r[0];
    assert(grown_ptr != 0u && grown_ptr != old_ptr);
    assert(re_guest_memory_resolve(&memory, grown_ptr, 16u,
        RE_GUEST_MEM_READ, &mapped) == RE_GUEST_MEM_OK);
    for (size_t i = 0; i < 16u; ++i) assert(((uint8_t *)mapped)[i] == 0x6du);
    call_import(&runtime, cxx_exports[2].value, guest_base + 0xe0u,
                grown_ptr, 0u);

    /* valloc returns page-aligned guest memory; realloc(NULL,n) is malloc. */
    call_import(&runtime, libc_exports[3].value, guest_base + 0x100u, 32u, 0u);
    const uint32_t page_ptr = runtime.cpu.r[0];
    assert(page_ptr != 0u && (page_ptr & (RE_GUEST_HEAP_PAGE_ALIGNMENT - 1u)) == 0u);
    call_import(&runtime, cxx_exports[3].value, guest_base + 0x120u,
                page_ptr, 0u);

    call_import(&runtime, libc_exports[2].value, guest_base + 0x140u, 0u, 48u);
    const uint32_t realloc_null = runtime.cpu.r[0];
    assert(realloc_null != 0u);
    call_import(&runtime, libc_exports[2].value, guest_base + 0x160u,
                realloc_null, 0u);
    assert(runtime.cpu.r[0] == 0u);
    call_import(&runtime, libc_exports[1].value, guest_base + 0x180u, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u);

    /* Out-of-space allocations fail closed; unknown frees do not corrupt the heap. */
    call_import(&runtime, libc_exports[0].value, guest_base + 0x1a0u,
                0x9000u, 0u);
    assert(runtime.cpu.r[0] == 0u);
    call_import(&runtime, libc_exports[1].value, guest_base + 0x1c0u,
                guest_base + 0x9000u, 0u);
    assert(runtime.cpu.r[0] == 0u);

    re_guest_cxx_heap_provider_shutdown(&cxx);
    puts("Shared guest heap/libc allocation tests: PASS");
    return 0;
}
