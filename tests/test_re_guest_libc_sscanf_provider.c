#include "re_guest_libc_sscanf_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put_string(uint8_t *guest, size_t offset, const char *value) {
    memcpy(guest + offset, value, strlen(value) + 1u);
}
static void put_u32(uint8_t *guest, size_t offset, uint32_t value) {
    guest[offset] = (uint8_t)value;
    guest[offset + 1u] = (uint8_t)(value >> 8);
    guest[offset + 2u] = (uint8_t)(value >> 16);
    guest[offset + 3u] = (uint8_t)(value >> 24);
}
static uint32_t get_u32(const uint8_t *guest, size_t offset) {
    return (uint32_t)guest[offset] |
        ((uint32_t)guest[offset + 1u] << 8) |
        ((uint32_t)guest[offset + 2u] << 16) |
        ((uint32_t)guest[offset + 3u] << 24);
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
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTLIBCSSCANFProvider provider;
    REGUESTELF32LINKSYMBOL symbols[2] = {0};
    REGUESTSYMBOL exports[1];
    size_t export_count = 0u, failed = 0u;
    double parsed_double = 0.0;

    (void)strcpy(symbols[1].name, "sscanf");
    symbols[1].section_index = 0u;
    symbols[1].binding = RE_GUEST_STB_GLOBAL;
    symbols[1].type = 2u;
    symbols[1].visibility = RE_GUEST_STV_DEFAULT;

    put_string(guest, 0x100u, " -42 hello 2a");
    put_string(guest, 0x180u, "AZ");
    put_string(guest, 0x1a0u, "1 7");
    put_string(guest, 0x1c0u, "42");
    put_string(guest, 0x1e0u, "3.125");
    put_string(guest, 0x200u, "%d %s %x");
    put_string(guest, 0x220u, "%2c");
    put_string(guest, 0x240u, "%*d %d");
    put_string(guest, 0x260u, "%d%n");
    put_string(guest, 0x280u, "%lf");

    assert(re_guest_libc_sscanf_provider_init(&provider) ==
           RE_GUEST_LIBC_SSCANF_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_libc_sscanf_provider_bind_elf_symbols(&provider, &imports,
        symbols, 2u, exports, 1u, &export_count, &failed) ==
        RE_GUEST_LIBC_SSCANF_OK);
    assert(export_count == 1u && failed == 2u);
    assert(strcmp(exports[0].name, "sscanf") == 0);
    assert(strcmp(exports[0].version_library, "libc.so") == 0);

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 base, 1);

    /* Three destinations: r2, r3, then the first stack word. */
    put_u32(guest, 0x3000u, base + 0x340u);
    call_import(&runtime, exports[0].value, base + 0x20u,
        base + 0x100u, base + 0x200u, base + 0x300u, base + 0x320u);
    assert(runtime.cpu.r[0] == 3u);
    assert((int32_t)get_u32(guest, 0x300u) == -42);
    assert(strcmp((char *)guest + 0x320u, "hello") == 0);
    assert(get_u32(guest, 0x340u) == 0x2au);

    /* %c does not skip whitespace or append a NUL terminator. */
    call_import(&runtime, exports[0].value, base + 0x40u,
        base + 0x180u, base + 0x220u, base + 0x360u, 0u);
    assert(runtime.cpu.r[0] == 1u);
    assert(memcmp(guest + 0x360u, "AZ", 2u) == 0);

    /* Assignment suppression consumes input but no destination argument. */
    call_import(&runtime, exports[0].value, base + 0x60u,
        base + 0x1a0u, base + 0x240u, base + 0x380u, 0u);
    assert(runtime.cpu.r[0] == 1u);
    assert(get_u32(guest, 0x380u) == 7u);

    /* %n stores consumed bytes but does not increment the assignment count. */
    call_import(&runtime, exports[0].value, base + 0x80u,
        base + 0x1c0u, base + 0x260u, base + 0x3a0u, base + 0x3a4u);
    assert(runtime.cpu.r[0] == 1u);
    assert(get_u32(guest, 0x3a0u) == 42u);
    assert(get_u32(guest, 0x3a4u) == 2u);

    /* %lf stores an eight-byte double; default %f would store float. */
    call_import(&runtime, exports[0].value, base + 0xa0u,
        base + 0x1e0u, base + 0x280u, base + 0x3b0u, 0u);
    assert(runtime.cpu.r[0] == 1u);
    memcpy(&parsed_double, guest + 0x3b0u, sizeof(parsed_double));
    assert(parsed_double == 3.125);

    /* Input failure before the first assignment returns EOF (-1). */
    put_string(guest, 0x2a0u, "");
    call_import(&runtime, exports[0].value, base + 0xc0u,
        base + 0x2a0u, base + 0x200u, base + 0x3c0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == -1);

    puts("Guest libc sscanf provider tests: PASS");
    return 0;
}
