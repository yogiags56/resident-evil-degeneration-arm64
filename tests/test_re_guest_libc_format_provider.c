#include "re_guest_libc_format_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct { char bytes[256]; size_t length; unsigned calls; } TestSink;
static int test_output_sink(const char *bytes, size_t length, void *user) {
    TestSink *sink = (TestSink *)user;
    if (!sink || length >= sizeof(sink->bytes)) return -1;
    memcpy(sink->bytes, bytes, length);
    sink->bytes[length] = '\\0';
    sink->length = length;
    ++sink->calls;
    return (int)length;
}

static void put_string(uint8_t *guest, size_t offset, const char *value) {
    memcpy(guest + offset, value, strlen(value) + 1u);
}
static void put_u32(uint8_t *guest, size_t offset, uint32_t value) {
    guest[offset] = (uint8_t)value;
    guest[offset + 1u] = (uint8_t)(value >> 8);
    guest[offset + 2u] = (uint8_t)(value >> 16);
    guest[offset + 3u] = (uint8_t)(value >> 24);
}
static void call_import(REGUESTRuntime *runtime, uint32_t pointer,
                        uint32_t return_address, uint32_t r0,
                        uint32_t r1, uint32_t r2, uint32_t r3) {
    runtime->cpu.r[15] = pointer & ~1u;
    runtime->cpu.thumb = 1u;
    runtime->cpu.r[14] = return_address | 1u;
    runtime->cpu.r[13] = 0x10003000u;
    runtime->cpu.r[0] = r0;
    runtime->cpu.r[1] = r1;
    runtime->cpu.r[2] = r2;
    runtime->cpu.r[3] = r3;
    assert(re_guest_runtime_step(runtime) == RE_RUNTIME_OK);
    assert(runtime->cpu.r[15] == return_address);
}

int main(void) {
    static uint8_t guest[0x4000];
    const uint32_t base = 0x10000000u;
    static const char *const names[RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT] = {
        "snprintf", "sprintf", "printf"
    };
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTLIBCFormatProvider provider;
    REGUESTELF32LINKSYMBOL symbols[4] = {0};
    REGUESTSYMBOL exports[3];
    TestSink sink = {{0}, 0u, 0u};
    size_t export_count = 0u, failed = 0u;

    (void)strcpy(symbols[1].name, names[0]);
    (void)strcpy(symbols[2].name, names[1]);
    for (size_t i = 1u; i <= 3u; ++i) {
        symbols[i].section_index = 0u;
        symbols[i].binding = RE_GUEST_STB_GLOBAL;
        symbols[i].type = 2u;
        symbols[i].visibility = RE_GUEST_STV_DEFAULT;
    }

    put_string(guest, 0x100u, "hi");
    put_string(guest, 0x120u, "guest");
    put_string(guest, 0x200u, "%s:%d");
    put_string(guest, 0x220u, "%s/%u");
    put_string(guest, 0x240u, "%lld");
    put_string(guest, 0x260u, "%*d");
    put_string(guest, 0x2a0u, "%s:%d");

    assert(re_guest_libc_format_provider_init(&provider) ==
           RE_GUEST_LIBC_FORMAT_OK);
    assert(re_guest_libc_format_provider_set_output(&provider,
        test_output_sink, &sink) == RE_GUEST_LIBC_FORMAT_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_libc_format_provider_bind_elf_symbols(&provider, &imports,
        symbols, 4u, exports, 3u, &export_count, &failed) ==
        RE_GUEST_LIBC_FORMAT_OK);
    assert(export_count == 3u && failed == 4u);
    assert(strcmp(exports[0].name, "snprintf") == 0);
    assert(strcmp(exports[1].name, "sprintf") == 0);
    assert(strcmp(exports[2].name, "printf") == 0);
    assert(strcmp(exports[0].version_library, "libc.so") == 0);

    /* Missing/ambiguous symbol diagnostics must be deterministic and atomic. */
    {
        REGUESTELF32LINKSYMBOL incomplete[3] = {0};
        REGUESTIMPORTTable rejected;
        REGUESTSYMBOL before[3];
        size_t rejected_count = 99u;
        size_t rejected_index = 99u;
        memcpy(before, exports, sizeof(before));
        (void)strcpy(incomplete[1].name, "snprintf");
        incomplete[1].section_index = 0u;
        incomplete[1].binding = RE_GUEST_STB_GLOBAL;
        incomplete[1].type = 2u;
        incomplete[1].visibility = RE_GUEST_STV_DEFAULT;
        assert(re_guest_import_table_init(&rejected, 0x31000000u) == RE_IMPORT_OK);
        assert(re_guest_libc_format_provider_bind_elf_symbols(&provider,
            &rejected, incomplete, 3u, exports, 3u, &rejected_count,
            &rejected_index) == RE_GUEST_LIBC_FORMAT_E_SYMBOL);
        assert(rejected.count == 0u && rejected_count == 0u);
        assert(rejected_index == 3u);
        assert(memcmp(exports, before, sizeof(before)) == 0);

        (void)strcpy(incomplete[0].name, "snprintf");
        incomplete[0].section_index = 0u;
        incomplete[0].binding = RE_GUEST_STB_GLOBAL;
        incomplete[0].type = 2u;
        incomplete[0].visibility = RE_GUEST_STV_DEFAULT;
        /* Duplicate the exact requested name at two distinct ELF indices. */
        (void)strcpy(incomplete[1].name, "snprintf");
        assert(re_guest_libc_format_provider_bind_elf_symbols(&provider,
            &rejected, incomplete, 3u, exports, 3u, &rejected_count,
            &rejected_index) == RE_GUEST_LIBC_FORMAT_E_SYMBOL);
        assert(rejected.count == 0u && rejected_count == 0u);
        assert(rejected_index == 2u);
        assert(memcmp(exports, before, sizeof(before)) == 0);
    }

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 base, 1);

    /* snprintf: first variadic word is r3; next word is on the guest stack. */
    put_u32(guest, 0x3000u, 42u);
    call_import(&runtime, exports[0].value, base + 0x20u,
        base + 0x400u, 32u, base + 0x200u, base + 0x100u);
    assert(runtime.cpu.r[0] == 5u);
    assert(strcmp((char *)guest + 0x400u, "hi:42") == 0);

    /* Truncation writes size-1 bytes plus NUL but returns full length. */
    call_import(&runtime, exports[0].value, base + 0x40u,
        base + 0x500u, 4u, base + 0x200u, base + 0x100u);
    assert(runtime.cpu.r[0] == 5u);
    assert(strcmp((char *)guest + 0x500u, "hi:") == 0);

    /* A zero-sized snprintf may use a null destination and performs no write. */
    call_import(&runtime, exports[0].value, base + 0x60u,
        0u, 0u, base + 0x200u, base + 0x100u);
    assert(runtime.cpu.r[0] == 5u);

    /* sprintf starts varargs at r2, unlike snprintf's r3 start. */
    call_import(&runtime, exports[1].value, base + 0x80u,
        base + 0x600u, base + 0x220u, base + 0x120u, 7u);
    assert(runtime.cpu.r[0] == 7u);
    assert(strcmp((char *)guest + 0x600u, "guest/7") == 0);

    /* 64-bit variadic arguments are aligned to an even word index. */
    call_import(&runtime, exports[1].value, base + 0xa0u,
        base + 0x700u, base + 0x240u, 0x71fb04cbu, 0x0000011fu);
    assert(runtime.cpu.r[0] == 13u);
    assert(strcmp((char *)guest + 0x700u, "1234567890123") == 0);

    /* Dynamic width consumes a core word before the stack-passed value. */
    put_u32(guest, 0x3000u, 7u);
    call_import(&runtime, exports[0].value, base + 0xc0u,
        base + 0x800u, 16u, base + 0x260u, 4u);
    assert(runtime.cpu.r[0] == 4u);
    assert(strcmp((char *)guest + 0x800u, "   7") == 0);

    /* printf routes the bounded output through an explicit host sink. */
    call_import(&runtime, exports[2].value, base + 0xe0u,
        base + 0x2a0u, base + 0x100u, 42u, 0u);
    assert(runtime.cpu.r[0] == 5u);
    assert(sink.calls == 1u && sink.length == 5u);
    assert(strcmp(sink.bytes, "hi:42") == 0);

    puts("Guest libc formatted-output provider tests: PASS");
    return 0;
}
