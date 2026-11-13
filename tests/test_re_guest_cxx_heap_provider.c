#include "re_guest_cxx_heap_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void call_import(REGUESTRuntime *runtime, uint32_t pointer,
                        uint32_t return_address, uint32_t r0) {
    runtime->cpu.r[15] = pointer & ~1u;
    runtime->cpu.thumb = 1u;
    runtime->cpu.r[14] = return_address | 1u;
    runtime->cpu.r[13] = 0x1000f000u;
    runtime->cpu.r[0] = r0;
    runtime->cpu.r[1] = 0u;
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
    REGUESTCXXHeapProvider provider;
    REGUESTELF32LINKSYMBOL symbols[5] = {0};
    REGUESTSYMBOL exports[4];
    size_t export_count = 0u, failed = 0u;
    void *first_bytes = NULL, *second_bytes = NULL, *reused_bytes = NULL;

    (void)strcpy(symbols[1].name, "_Znwj");
    (void)strcpy(symbols[2].name, "_Znaj");
    (void)strcpy(symbols[3].name, "_ZdlPv");
    (void)strcpy(symbols[4].name, "_ZdaPv");
    for (size_t i = 1u; i < 5u; ++i) {
        symbols[i].section_index = 0u;
        symbols[i].binding = RE_GUEST_STB_GLOBAL;
        symbols[i].type = 2u;
        symbols[i].visibility = RE_GUEST_STV_DEFAULT;
    }

    assert(re_guest_cxx_heap_provider_init(&provider, heap_base, 0x4000u) ==
           RE_GUEST_CXX_HEAP_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_cxx_heap_provider_bind_elf_symbols(&provider, &imports,
        symbols, 5u, exports, 4u, &export_count, &failed) ==
        RE_GUEST_CXX_HEAP_OK);
    assert(export_count == 4u && failed == 5u && imports.count == 4u);
    assert(strcmp(exports[0].name, "_Znwj") == 0);
    assert(strcmp(exports[1].name, "_Znaj") == 0);
    assert(strcmp(exports[2].name, "_ZdlPv") == 0);
    assert(strcmp(exports[3].name, "_ZdaPv") == 0);
    for (size_t i = 0; i < 4u; ++i)
        assert(strcmp(exports[i].version_library, "libstdc++.so") == 0);

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, guest_base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 guest_base, 1);

    call_import(&runtime, exports[0].value, guest_base + 0x20u, 17u);
    const uint32_t first = runtime.cpu.r[0];
    assert(first == heap_base && (first & 7u) == 0u);
    assert(re_guest_memory_resolve(&memory, first, 17u,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE, &first_bytes) == RE_GUEST_MEM_OK);
    memset(first_bytes, 0x5a, 17u);

    call_import(&runtime, exports[1].value, guest_base + 0x40u, 33u);
    const uint32_t second = runtime.cpu.r[0];
    assert(second > first && (second & 7u) == 0u);
    assert(re_guest_memory_resolve(&memory, second, 33u,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE, &second_bytes) == RE_GUEST_MEM_OK);
    memset(second_bytes, 0xa5, 33u);
    assert(second >= first + 24u);

    /* Scalar delete releases the exact block; first-fit reuses its address. */
    call_import(&runtime, exports[2].value, guest_base + 0x60u, first);
    assert(runtime.cpu.r[0] == 0u);
    call_import(&runtime, exports[0].value, guest_base + 0x80u, 24u);
    const uint32_t reused = runtime.cpu.r[0];
    assert(reused == first);
    assert(re_guest_memory_resolve(&memory, reused, 24u,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE, &reused_bytes) == RE_GUEST_MEM_OK);

    /* Array delete and scalar delete share the guest heap free-list records. */
    call_import(&runtime, exports[3].value, guest_base + 0xa0u, second);
    call_import(&runtime, exports[2].value, guest_base + 0xc0u, reused);
    assert(runtime.cpu.r[0] == 0u);

    /* Null and unknown delete pointers are harmless; oversized new fails closed. */
    call_import(&runtime, exports[2].value, guest_base + 0xe0u, 0u);
    call_import(&runtime, exports[2].value, guest_base + 0x100u, guest_base + 0x9000u);
    call_import(&runtime, exports[0].value, guest_base + 0x120u, 0x5000u);
    assert(runtime.cpu.r[0] == 0u);

    re_guest_cxx_heap_provider_shutdown(&provider);
    puts("Guest C++ heap provider tests: PASS");
    return 0;
}
