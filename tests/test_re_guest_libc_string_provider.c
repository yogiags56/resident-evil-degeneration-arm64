#include "re_guest_libc_string_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void call_import(REGUESTRuntime *runtime, uint32_t pointer,
                        uint32_t return_address, uint32_t r0,
                        uint32_t r1, uint32_t r2) {
    runtime->cpu.r[15] = pointer & ~1u;
    runtime->cpu.thumb = 1u;
    runtime->cpu.r[14] = return_address | 1u;
    runtime->cpu.r[13] = 0x1001f000u;
    runtime->cpu.r[0] = r0;
    runtime->cpu.r[1] = r1;
    runtime->cpu.r[2] = r2;
    runtime->cpu.r[3] = 0u;
    assert(re_guest_runtime_step(runtime) == RE_RUNTIME_OK);
    assert(runtime->cpu.r[15] == return_address);
}

static const char *guest_string(const uint8_t *guest, uint32_t base,
                                uint32_t address) {
    return (const char *)(guest + (address - base));
}

int main(void) {
    static uint8_t guest[0x20000];
    const uint32_t base = 0x10000000u;
    static const char *const names[RE_GUEST_LIBC_STRING_SYMBOL_COUNT] = {
        "memcpy", "memmove", "memset", "strlen", "strcmp", "strncmp",
        "strcpy", "strncpy", "strncat", "strchr", "strstr",
        "strcasecmp", "strncasecmp", "atoi", "strtok"
    };
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTLIBCStringProvider provider;
    REGUESTELF32LINKSYMBOL symbols[RE_GUEST_LIBC_STRING_SYMBOL_COUNT + 1u] = {0};
    REGUESTSYMBOL exports[RE_GUEST_LIBC_STRING_SYMBOL_COUNT];
    size_t export_count = 0u, failed = 0u;

    for (size_t i = 0; i < RE_GUEST_LIBC_STRING_SYMBOL_COUNT; ++i) {
        (void)strcpy(symbols[i + 1u].name, names[i]);
        symbols[i + 1u].section_index = 0u;
        symbols[i + 1u].binding = RE_GUEST_STB_GLOBAL;
        symbols[i + 1u].type = 2u;
        symbols[i + 1u].visibility = RE_GUEST_STV_DEFAULT;
    }

    (void)strcpy((char *)guest + 0x100u, "hello");
    (void)strcpy((char *)guest + 0x120u, "world");
    (void)strcpy((char *)guest + 0x140u, "Hello");
    (void)strcpy((char *)guest + 0x160u, "help");
    (void)strcpy((char *)guest + 0x180u, "hello world");
    (void)strcpy((char *)guest + 0x1a0u, "hi");
    (void)strcpy((char *)guest + 0x1c0u, "hello ");
    (void)strcpy((char *)guest + 0x1e0u, "  -1234xyz");
    (void)strcpy((char *)guest + 0x200u, "red,green,,blue");
    (void)strcpy((char *)guest + 0x240u, "abcdef");

    assert(re_guest_libc_string_provider_init(&provider) == RE_GUEST_LIBC_STRING_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_libc_string_provider_bind_elf_symbols(&provider, &imports,
        symbols, RE_GUEST_LIBC_STRING_SYMBOL_COUNT + 1u, exports,
        RE_GUEST_LIBC_STRING_SYMBOL_COUNT, &export_count, &failed) ==
        RE_GUEST_LIBC_STRING_OK);
    assert(export_count == RE_GUEST_LIBC_STRING_SYMBOL_COUNT);
    assert(failed == RE_GUEST_LIBC_STRING_SYMBOL_COUNT + 1u);
    assert(imports.count == RE_GUEST_LIBC_STRING_SYMBOL_COUNT);
    for (size_t i = 0; i < RE_GUEST_LIBC_STRING_SYMBOL_COUNT; ++i) {
        assert(strcmp(exports[i].name, names[i]) == 0);
        assert(strcmp(exports[i].version_library, "libc.so") == 0);
    }

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 base, 1);

    call_import(&runtime, exports[3].value, base + 0x20u, base + 0x100u, 0u, 0u);
    assert(runtime.cpu.r[0] == 5u);
    call_import(&runtime, exports[4].value, base + 0x40u, base + 0x100u, base + 0x120u, 0u);
    assert((int32_t)runtime.cpu.r[0] < 0);
    call_import(&runtime, exports[5].value, base + 0x60u, base + 0x100u, base + 0x160u, 3u);
    assert(runtime.cpu.r[0] == 0u);
    call_import(&runtime, exports[11].value, base + 0x80u, base + 0x100u, base + 0x140u, 0u);
    assert(runtime.cpu.r[0] == 0u);
    call_import(&runtime, exports[12].value, base + 0xa0u, base + 0x100u, base + 0x140u, 5u);
    assert(runtime.cpu.r[0] == 0u);

    call_import(&runtime, exports[0].value, base + 0xc0u, base + 0x300u, base + 0x100u, 6u);
    assert(runtime.cpu.r[0] == base + 0x300u);
    assert(strcmp(guest_string(guest, base, base + 0x300u), "hello") == 0);
    call_import(&runtime, exports[1].value, base + 0xe0u, base + 0x241u, base + 0x240u, 5u);
    assert(strcmp(guest_string(guest, base, base + 0x241u), "aabcd") == 0);
    call_import(&runtime, exports[2].value, base + 0x100u, base + 0x320u, 'A', 4u);
    assert(memcmp(guest + 0x320u, "AAAA", 4u) == 0);

    call_import(&runtime, exports[6].value, base + 0x120u, base + 0x340u, base + 0x100u, 0u);
    assert(runtime.cpu.r[0] == base + 0x340u);
    assert(strcmp(guest_string(guest, base, base + 0x340u), "hello") == 0);
    call_import(&runtime, exports[7].value, base + 0x140u, base + 0x360u, base + 0x1a0u, 5u);
    assert(memcmp(guest + 0x360u, "hi\0\0\0", 5u) == 0);
    call_import(&runtime, exports[8].value, base + 0x160u, base + 0x1c0u, base + 0x120u, 3u);
    assert(strcmp(guest_string(guest, base, base + 0x1c0u), "hello wor") == 0);

    call_import(&runtime, exports[9].value, base + 0x180u, base + 0x100u, 'l', 0u);
    assert(runtime.cpu.r[0] == base + 0x102u);
    call_import(&runtime, exports[10].value, base + 0x1a0u, base + 0x180u, base + 0x120u, 0u);
    assert(runtime.cpu.r[0] == base + 0x186u);
    call_import(&runtime, exports[13].value, base + 0x1c0u, base + 0x1e0u, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == -1234);

    call_import(&runtime, exports[14].value, base + 0x1e0u, base + 0x200u, base + 0x220u, 0u);
    assert(runtime.cpu.r[0] == base + 0x200u);
    assert(strcmp(guest_string(guest, base, runtime.cpu.r[0]), "red") == 0);
    call_import(&runtime, exports[14].value, base + 0x200u, 0u, base + 0x220u, 0u);
    assert(runtime.cpu.r[0] == base + 0x204u);
    assert(strcmp(guest_string(guest, base, runtime.cpu.r[0]), "green") == 0);
    call_import(&runtime, exports[14].value, base + 0x220u, 0u, base + 0x220u, 0u);
    assert(runtime.cpu.r[0] == base + 0x20bu);
    assert(strcmp(guest_string(guest, base, runtime.cpu.r[0]), "blue") == 0);
    call_import(&runtime, exports[14].value, base + 0x240u, 0u, base + 0x220u, 0u);
    assert(runtime.cpu.r[0] == 0u);

    puts("Guest libc memory/string provider tests: PASS");
    return 0;
}
