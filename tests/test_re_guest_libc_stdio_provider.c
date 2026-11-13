#include "re_guest_libc_stdio_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t bytes[128];
    size_t length;
    size_t position;
    uint8_t opened;
} FakeStream;

static void *fake_open(void *user, const char *path, const char *mode) {
    FakeStream *s = (FakeStream *)user;
    if (!s || strcmp(path, "data.bin") != 0 || !mode) return NULL;
    if (s->opened) return NULL;
    if (mode[0] == 'w') s->length = 0u;
    if (mode[0] == 'a') s->position = s->length;
    else s->position = 0u;
    s->opened = 1u;
    return s;
}
static int64_t fake_read(void *user, void *stream, void *dst, uint64_t bytes) {
    FakeStream *s = (FakeStream *)stream;
    size_t available, take;
    (void)user;
    if (!s || !s->opened || (!dst && bytes)) return -1;
    available = s->position < s->length ? s->length - s->position : 0u;
    take = bytes < available ? (size_t)bytes : available;
    if (take) memcpy(dst, s->bytes + s->position, take);
    s->position += take;
    return (int64_t)take;
}
static int64_t fake_write(void *user, void *stream, const void *src,
                          uint64_t bytes) {
    FakeStream *s = (FakeStream *)stream;
    size_t take;
    (void)user;
    if (!s || !s->opened || (!src && bytes) || bytes > sizeof(s->bytes))
        return -1;
    take = (size_t)bytes;
    if (s->position > sizeof(s->bytes) - take) return -1;
    if (take) memcpy(s->bytes + s->position, src, take);
    s->position += take;
    if (s->position > s->length) s->length = s->position;
    return (int64_t)take;
}
static int fake_seek(void *user, void *stream, int64_t offset, int whence) {
    FakeStream *s = (FakeStream *)stream;
    int64_t base, target;
    (void)user;
    if (!s || !s->opened) return -1;
    base = whence == 0 ? 0 : (whence == 1 ? (int64_t)s->position :
           (whence == 2 ? (int64_t)s->length : -1));
    if (base < 0 || offset < -base) return -1;
    target = base + offset;
    if (target < 0 || target > (int64_t)sizeof(s->bytes)) return -1;
    s->position = (size_t)target;
    return 0;
}
static int64_t fake_tell(void *user, void *stream) {
    FakeStream *s = (FakeStream *)stream;
    (void)user;
    return s && s->opened ? (int64_t)s->position : -1;
}
static int fake_flush(void *user, void *stream) {
    FakeStream *s = (FakeStream *)stream;
    (void)user;
    return s && s->opened ? 0 : -1;
}
static int fake_close(void *user, void *stream) {
    FakeStream *s = (FakeStream *)stream;
    (void)user;
    if (!s || !s->opened) return -1;
    s->opened = 0u;
    return 0;
}
static void put_string(uint8_t *guest, size_t offset, const char *s) {
    memcpy(guest + offset, s, strlen(s) + 1u);
}
static void call_import(REGUESTRuntime *runtime, uint32_t pointer,
                        uint32_t return_address, uint32_t r0, uint32_t r1,
                        uint32_t r2, uint32_t r3) {
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
    static const char *const names[RE_GUEST_LIBC_STDIO_SYMBOL_COUNT] = {
        "fopen", "fclose", "fread", "fwrite", "fseek", "ftell", "fgets",
        "fprintf", "feof", "ferror", "fflush"
    };
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTLIBCStdioProvider provider;
    REGUESTLIBCStdioBackend backend = {
        fake_open, fake_read, fake_write, fake_seek, fake_tell, fake_close, fake_flush
    };
    REGUESTELF32LINKSYMBOL symbols[12] = {0};
    REGUESTSYMBOL exports[RE_GUEST_LIBC_STDIO_SYMBOL_COUNT];
    FakeStream stream = {{'a','b','c','d','e','f'}, 6u, 0u, 0u};
    size_t export_count = 0u, failed = 0u;
    uint32_t handle;

    for (size_t i = 0; i < RE_GUEST_LIBC_STDIO_SYMBOL_COUNT; ++i) {
        (void)strcpy(symbols[i + 1u].name, names[i]);
        symbols[i + 1u].section_index = 0u;
        symbols[i + 1u].binding = RE_GUEST_STB_GLOBAL;
        symbols[i + 1u].type = 2u;
        symbols[i + 1u].visibility = RE_GUEST_STV_DEFAULT;
    }
    put_string(guest, 0x100u, "data.bin");
    put_string(guest, 0x120u, "r+");
    put_string(guest, 0x600u, "XY");
    put_string(guest, 0x700u, "id=%d name=%s\n");
    put_string(guest, 0x740u, "guest");

    assert(re_guest_libc_stdio_provider_init(&provider, &backend, &stream) ==
           RE_GUEST_LIBC_STDIO_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_libc_stdio_provider_bind_elf_symbols(&provider, &imports,
        symbols, 12u, exports, RE_GUEST_LIBC_STDIO_SYMBOL_COUNT,
        &export_count, &failed) == RE_GUEST_LIBC_STDIO_OK);
    assert(export_count == RE_GUEST_LIBC_STDIO_SYMBOL_COUNT && failed == 12u);

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 base, 1);

    call_import(&runtime, exports[0].value, base + 0x20u,
        base + 0x100u, base + 0x120u, 0u, 0u);
    handle = runtime.cpu.r[0];
    assert(handle != 0u && stream.opened);

    /* fread(ptr, size=2, count=2, stream) returns complete item count. */
    call_import(&runtime, exports[2].value, base + 0x40u,
        base + 0x400u, 2u, 2u, handle);
    assert(runtime.cpu.r[0] == 2u);
    assert(memcmp(guest + 0x400u, "abcd", 4u) == 0);

    /* fseek(FILE*, offset, SEEK_SET) uses r0/r1/r2 on AAPCS32. */
    call_import(&runtime, exports[4].value, base + 0x60u,
        handle, 1u, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == 0);
    call_import(&runtime, exports[5].value, base + 0x80u,
        handle, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 1u);

    /* fgets(buffer, 4, FILE*) reads through newline/limit and NUL-terminates. */
    call_import(&runtime, exports[6].value, base + 0xa0u,
        base + 0x500u, 4u, handle, 0u);
    assert(runtime.cpu.r[0] == base + 0x500u);
    assert(strcmp((char *)guest + 0x500u, "bcd") == 0);

    /* fwrite returns complete item count and updates the backend stream. */
    call_import(&runtime, exports[3].value, base + 0xc0u,
        base + 0x600u, 1u, 2u, handle);
    assert(runtime.cpu.r[0] == 2u);
    assert(memcmp(stream.bytes, "abcdXY", 6u) == 0);

    /* fprintf uses the shared AAPCS32 formatter and writes through backend. */
    call_import(&runtime, exports[7].value, base + 0xd0u,
        handle, base + 0x700u, 17u, base + 0x740u);
    assert(runtime.cpu.r[0] == 17u);
    assert(memcmp(stream.bytes + 6u, "id=17 name=guest\n", 17u) == 0);

    /* Reading beyond the current end sets EOF; a successful seek clears it. */
    call_import(&runtime, exports[4].value, base + 0xd2u,
        handle, 0, 2u, 0u);
    assert((int32_t)runtime.cpu.r[0] == 0);
    call_import(&runtime, exports[2].value, base + 0xd4u,
        base + 0x900u, 1u, 1u, handle);
    assert(runtime.cpu.r[0] == 0u);
    call_import(&runtime, exports[8].value, base + 0xd6u,
        handle, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 1u);
    call_import(&runtime, exports[4].value, base + 0xd7u,
        handle, 0, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == 0);
    call_import(&runtime, exports[8].value, base + 0xd9u,
        handle, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u);

    /* fflush and stream status imports use the same opaque FILE token. */
    call_import(&runtime, exports[10].value, base + 0xd8u,
        handle, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u);
    call_import(&runtime, exports[8].value, base + 0xdau,
        handle, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u);
    call_import(&runtime, exports[9].value, base + 0xdcu,
        handle, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u);

    call_import(&runtime, exports[1].value, base + 0xe0u,
        handle, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u && !stream.opened);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    puts("Guest libc stdio provider tests: PASS");
    return 0;
}
