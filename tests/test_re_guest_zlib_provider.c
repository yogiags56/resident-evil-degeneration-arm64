#include "re_guest_zlib_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

static void put_u32(uint8_t *bytes, size_t offset, uint32_t value) {
    bytes[offset] = (uint8_t)value;
    bytes[offset + 1u] = (uint8_t)(value >> 8);
    bytes[offset + 2u] = (uint8_t)(value >> 16);
    bytes[offset + 3u] = (uint8_t)(value >> 24);
}

static uint32_t get_u32(const uint8_t *bytes, size_t offset) {
    return (uint32_t)bytes[offset] |
        ((uint32_t)bytes[offset + 1u] << 8) |
        ((uint32_t)bytes[offset + 2u] << 16) |
        ((uint32_t)bytes[offset + 3u] << 24);
}

static void call_import(REGUESTRuntime *runtime, uint32_t pointer,
                        uint32_t return_address, uint32_t r0,
                        uint32_t r1, uint32_t r2, uint32_t r3) {
    runtime->cpu.r[15] = pointer & ~1u;
    runtime->cpu.thumb = 1u;
    runtime->cpu.r[14] = return_address | 1u;
    runtime->cpu.r[13] = 0x1000f000u;
    runtime->cpu.r[0] = r0;
    runtime->cpu.r[1] = r1;
    runtime->cpu.r[2] = r2;
    runtime->cpu.r[3] = r3;
    assert(re_guest_runtime_step(runtime) == RE_RUNTIME_OK);
    assert(runtime->cpu.r[15] == return_address);
}

int main(void) {
    static uint8_t guest[0x10000];
    const uint8_t original[] =
        "Resident Evil Degeneration ARM32 guest zlib compatibility regression";
    uint8_t compressed[1024];
    uLongf compressed_size = (uLongf)sizeof(compressed);
    const uint32_t guest_base = 0x10000000u;
    const uint32_t stream_a = guest_base + 0x1000u;
    const uint32_t stream_b = guest_base + 0x1100u;
    const uint32_t version = guest_base + 0x80u;
    const uint32_t input = guest_base + 0x200u;
    const uint32_t output = guest_base + 0x3000u;
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTZLIBProvider provider;
    REGUESTELF32LINKSYMBOL symbols[5] = {0};
    REGUESTSYMBOL exports[4];
    size_t export_count = 0u, failed = 0u;

    assert(compress2(compressed, &compressed_size, original,
        (uLong)sizeof(original), Z_BEST_COMPRESSION) == Z_OK);
    memcpy(guest + 0x80u, ZLIB_VERSION, strlen(ZLIB_VERSION) + 1u);
    memcpy(guest + 0x200u, compressed, (size_t)compressed_size);

    (void)strcpy(symbols[1].name, "inflate");
    (void)strcpy(symbols[2].name, "inflateEnd");
    (void)strcpy(symbols[3].name, "inflateInit2_");
    (void)strcpy(symbols[4].name, "inflateInit_");
    for (size_t i = 1u; i < 5u; ++i) {
        symbols[i].section_index = 0u;
        symbols[i].binding = RE_GUEST_STB_GLOBAL;
        symbols[i].type = 2u;
        symbols[i].visibility = RE_GUEST_STV_DEFAULT;
    }

    assert(re_guest_zlib_provider_init(&provider, 0x70000000u) ==
           RE_GUEST_ZLIB_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_zlib_provider_bind_elf_symbols(&provider, &imports,
        symbols, 5u, exports, 4u, &export_count, &failed) ==
        RE_GUEST_ZLIB_OK);
    assert(export_count == 4u && failed == 5u && imports.count == 4u);
    assert(strcmp(exports[0].name, "inflate") == 0);
    assert(strcmp(exports[1].name, "inflateEnd") == 0);
    assert(strcmp(exports[2].name, "inflateInit2_") == 0);
    assert(strcmp(exports[3].name, "inflateInit_") == 0);
    for (size_t i = 0; i < 4u; ++i)
        assert(strcmp(exports[i].version_library, "libz.so") == 0);

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, guest_base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 guest_base, 1);

    /* inflateInit_ uses the exact 56-byte ARM32 z_stream layout. */
    call_import(&runtime, exports[3].value, guest_base + 0x20u,
                stream_a, version, RE_GUEST_ZLIB_STREAM32_SIZE, 0u);
    assert((int32_t)runtime.cpu.r[0] == Z_OK);
    assert(get_u32(guest, 0x1000u + 28u) == 0x70000000u);

    put_u32(guest, 0x1000u + 0u, input);
    put_u32(guest, 0x1000u + 4u, (uint32_t)compressed_size);
    put_u32(guest, 0x1000u + 8u, 0u);
    put_u32(guest, 0x1000u + 12u, output);
    put_u32(guest, 0x1000u + 16u, 0x1000u);
    put_u32(guest, 0x1000u + 20u, 0u);

    call_import(&runtime, exports[0].value, guest_base + 0x40u,
                stream_a, Z_FINISH, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == Z_STREAM_END);
    assert(get_u32(guest, 0x1000u + 4u) == 0u);
    assert(get_u32(guest, 0x1000u + 8u) == (uint32_t)compressed_size);
    assert(get_u32(guest, 0x1000u + 20u) == (uint32_t)sizeof(original));
    assert(get_u32(guest, 0x1000u + 16u) ==
           0x1000u - (uint32_t)sizeof(original));
    assert(memcmp(guest + 0x3000u, original, sizeof(original)) == 0);

    call_import(&runtime, exports[1].value, guest_base + 0x60u,
                stream_a, 0u, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == Z_OK);
    assert(get_u32(guest, 0x1000u + 28u) == 0u);

    /* Exercise inflateInit2_ independently with an explicit window size. */
    call_import(&runtime, exports[2].value, guest_base + 0x80u,
                stream_b, MAX_WBITS, version,
                RE_GUEST_ZLIB_STREAM32_SIZE);
    assert((int32_t)runtime.cpu.r[0] == Z_OK);
    call_import(&runtime, exports[1].value, guest_base + 0xa0u,
                stream_b, 0u, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == Z_OK);

    /* Invalid guest stream size must fail before allocating native state. */
    call_import(&runtime, exports[3].value, guest_base + 0xc0u,
                guest_base + 0x1200u, version,
                RE_GUEST_ZLIB_STREAM32_SIZE - 1u, 0u);
    assert((int32_t)runtime.cpu.r[0] == Z_VERSION_ERROR);

    /* Unknown state tokens are rejected without touching host zlib state. */
    put_u32(guest, 0x1000u + 28u, 0x7ffffff0u);
    call_import(&runtime, exports[0].value, guest_base + 0xe0u,
                stream_a, Z_NO_FLUSH, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == Z_STREAM_ERROR);
    put_u32(guest, 0x1000u + 28u, 0u);

    re_guest_zlib_provider_shutdown(&provider);
    puts("Guest libz provider tests: PASS");
    return 0;
}
