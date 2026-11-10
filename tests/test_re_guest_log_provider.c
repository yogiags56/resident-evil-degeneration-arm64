#include "re_guest_log_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    int calls;
    int priority;
    char tag[RE_GUEST_LOG_TAG_CAP];
    char message[RE_GUEST_LOG_MESSAGE_CAP];
} MockLog;

static int mock_write(int priority, const char *tag, const char *message,
                      void *user) {
    MockLog *mock = (MockLog *)user;
    ++mock->calls;
    mock->priority = priority;
    snprintf(mock->tag, sizeof(mock->tag), "%s", tag);
    snprintf(mock->message, sizeof(mock->message), "%s", message);
    return (int)strlen(message);
}

static void call_import(REGUESTRuntime *runtime, uint32_t pointer,
                        uint32_t return_address, uint32_t r0,
                        uint32_t r1, uint32_t r2, uint32_t r3) {
    runtime->cpu.r[15] = pointer & ~1u;
    runtime->cpu.thumb = 1u;
    runtime->cpu.r[14] = return_address | 1u;
    runtime->cpu.r[13] = 0x10001800u;
    runtime->cpu.r[0] = r0;
    runtime->cpu.r[1] = r1;
    runtime->cpu.r[2] = r2;
    runtime->cpu.r[3] = r3;
    assert(re_guest_runtime_step(runtime) == RE_RUNTIME_OK);
    assert(runtime->cpu.r[15] == return_address);
}

static void put_string(uint8_t *bytes, size_t offset, const char *value) {
    memcpy(bytes + offset, value, strlen(value) + 1u);
}

static void put_u32(uint8_t *bytes, size_t offset, uint32_t value) {
    bytes[offset] = (uint8_t)value;
    bytes[offset + 1u] = (uint8_t)(value >> 8);
    bytes[offset + 2u] = (uint8_t)(value >> 16);
    bytes[offset + 3u] = (uint8_t)(value >> 24);
}

int main(void) {
    static uint8_t bytes[0x2000];
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports, rejected;
    REGUESTRuntime runtime;
    REGUESTLOGProvider provider;
    REGUESTELF32LINKSYMBOL elf_symbols[3] = {0};
    REGUESTSYMBOL exports[2];
    MockLog mock = {0};
    size_t export_count = 0, failed = 0;
    uint32_t print_pointer = 0, write_pointer = 0;

    strcpy(elf_symbols[1].name, "__android_log_print");
    elf_symbols[1].section_index = 0u;
    elf_symbols[1].binding = RE_GUEST_STB_GLOBAL;
    elf_symbols[1].type = 2u;
    elf_symbols[1].visibility = RE_GUEST_STV_DEFAULT;
    strcpy(elf_symbols[2].name, "__android_log_write");
    elf_symbols[2].section_index = 0u;
    elf_symbols[2].binding = RE_GUEST_STB_GLOBAL;
    elf_symbols[2].type = 2u;
    elf_symbols[2].visibility = RE_GUEST_STV_DEFAULT;

    provider.write_callback = mock_write;
    provider.user = &mock;
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_log_provider_bind_elf_symbols(
        &provider, &imports, elf_symbols, 3u, exports, 2u,
        &export_count, &failed) == RE_GUEST_LOG_OK);
    assert(export_count == 2u && failed == 3u);
    assert(imports.count == 2u);
    assert(strcmp(exports[0].name, "__android_log_print") == 0);
    assert(strcmp(exports[1].name, "__android_log_write") == 0);
    assert(exports[0].value == imports.entries[0].trampoline_address + 1u ||
           exports[0].value == imports.entries[1].trampoline_address + 1u);

    /* Missing relocation-backed symbol is fail-closed and transactional. */
    {
        REGUESTELF32LINKSYMBOL incomplete[2] = {
            {0}, {0}
        };
        REGUESTSYMBOL before[2];
        REGUESTIMPORTTable backup = imports;
        memcpy(before, exports, sizeof(before));
        strcpy(incomplete[1].name, "__android_log_write");
        incomplete[1].binding = RE_GUEST_STB_GLOBAL;
        incomplete[1].visibility = RE_GUEST_STV_DEFAULT;
        assert(re_guest_import_table_init(&rejected, 0x31000000u) == RE_IMPORT_OK);
        assert(re_guest_log_provider_bind_elf_symbols(
            &provider, &rejected, incomplete, 2u, exports, 2u,
            &export_count, &failed) == RE_GUEST_LOG_E_SYMBOL);
        assert(rejected.count == 0u && export_count == 0u);
        assert(memcmp(exports, before, sizeof(before)) == 0);
        assert(imports.count == backup.count);
    }

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, 0x10000000u, sizeof(bytes), bytes,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    put_string(bytes, 0x100u, "Degeneration");
    put_string(bytes, 0x180u, "id=%d name=%s code=%08x %%");
    put_string(bytes, 0x240u, "hello from guest");

    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 0x10000000u, 1);

    /* printf arguments begin at core word 3: r3, then stack words. */
    put_u32(bytes, 0x800u, 0x10000240u); /* guest %s pointer */
    put_u32(bytes, 0x804u, 0x89abcdefu); /* guest %08x */
    runtime.cpu.r[3] = 42u;
    /* r3 is word 3: %d. Remaining words are at guest SP. */
    runtime.cpu.r[13] = 0x10000800u;
    runtime.cpu.r[15] = exports[0].value & ~1u;
    runtime.cpu.thumb = 1u;
    runtime.cpu.r[14] = 0x10000020u | 1u;
    runtime.cpu.r[0] = 4u;
    runtime.cpu.r[1] = 0x10000100u;
    runtime.cpu.r[2] = 0x10000180u;
    assert(re_guest_runtime_step(&runtime) == RE_RUNTIME_OK);
    assert(mock.calls == 1 && mock.priority == 4);
    assert(strcmp(mock.tag, "Degeneration") == 0);
    assert(strcmp(mock.message, "id=42 name=hello from guest code=89abcdef %") == 0);

    /* Fixed-width __android_log_write uses r0/r1/r2 only. */
    call_import(&runtime, exports[1].value, 0x10000040u,
                5u, 0x10000100u, 0x10000240u, 0u);
    assert(mock.calls == 2 && mock.priority == 5);
    assert(strcmp(mock.message, "hello from guest") == 0);

    puts("Guest liblog provider tests: PASS");
    return 0;
}
