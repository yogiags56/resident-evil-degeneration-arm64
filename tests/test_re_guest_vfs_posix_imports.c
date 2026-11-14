#include "re_guest_vfs_posix_imports.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void invoke(REGUESTRuntime *runtime, uint32_t pointer,
                   uint32_t return_address, uint32_t sp,
                   uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3) {
    runtime->cpu.r[15] = pointer & ~1u;
    runtime->cpu.thumb = 1u;
    runtime->cpu.r[14] = return_address | 1u;
    runtime->cpu.r[13] = sp;
    runtime->cpu.r[0] = r0;
    runtime->cpu.r[1] = r1;
    runtime->cpu.r[2] = r2;
    runtime->cpu.r[3] = r3;
    assert(re_guest_runtime_step(runtime) == RE_RUNTIME_OK);
    assert(runtime->cpu.r[15] == return_address);
}

int main(void) {
    static uint8_t guest[0x10000];
    static const uint8_t payload[] = "resident evil asset";
    const uint32_t base = 0x10000000u;
    const uint32_t stack = base + 0x8000u;
    const uint32_t errno_addr = base + 0x100u;
    const uint32_t path_addr = base + 0x200u;
    const uint32_t data_addr = base + 0x400u;
    re_dtrz_entry entry;
    re_dtrz_archive archive;
    re_dtrz_vfs vfs;
    REGUESTVFSPOSIX fs;
    REGUESTVFSPOSIXImportProvider provider;
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTELF32LINKSYMBOL symbols[6] = {0};
    REGUESTSYMBOL exports[RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT];
    size_t export_count = 0u, failed = 0u;
    int fd;

    memset(&entry, 0, sizeof(entry));
    entry.path = (char *)"data/game.bin";
    entry.offset = 0u;
    entry.packed_size = sizeof(payload) - 1u;
    entry.decoded_size_a = (uint32_t)(sizeof(payload) - 1u);
    entry.decoded_size_b = entry.decoded_size_a;
    entry.method = RE_DTRZ_METHOD_STORED;
    memset(&archive, 0, sizeof(archive));
    archive.bytes = payload;
    archive.byte_count = sizeof(payload) - 1u;
    archive.file_count = 1u;
    archive.entries = &entry;
    assert(re_dtrz_vfs_init(&vfs, &archive, NULL, NULL) == RE_DTRZ_OK);
    assert(re_guest_vfs_posix_init(&fs, &vfs) == RE_GUEST_VFS_POSIX_OK);

    (void)strcpy((char *)(guest + (path_addr - base)), "data/game.bin");
    for (size_t i = 0; i < RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT; ++i) {
        (void)strcpy(symbols[i + 1u].name,
                     (const char *[]){"open","read","lseek","close","access"}[i]);
        symbols[i + 1u].section_index = 0u;
        symbols[i + 1u].binding = RE_GUEST_STB_GLOBAL;
        symbols[i + 1u].type = 2u;
        symbols[i + 1u].visibility = RE_GUEST_STV_DEFAULT;
    }

    assert(re_guest_vfs_posix_import_provider_init(&provider, &fs, errno_addr) ==
           RE_GUEST_VFS_POSIX_IMPORT_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_vfs_posix_import_provider_bind_elf_symbols(
        &provider, &imports, symbols, 6u, exports,
        RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT, &export_count, &failed) ==
        RE_GUEST_VFS_POSIX_IMPORT_OK);
    assert(export_count == RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT);
    for (size_t i = 0; i < export_count; ++i)
        assert(strcmp(exports[i].version_library, "libc.so") == 0);

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, base, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 base, 1);

    invoke(&runtime, exports[0].value, base + 0x20u, stack,
           path_addr, RE_GUEST_VFS_POSIX_O_RDONLY, 0u, 0u);
    fd = (int)runtime.cpu.r[0];
    assert(fd == 3 && provider.last_errno == RE_GUEST_VFS_POSIX_ERRNO_NONE);

    invoke(&runtime, exports[1].value, base + 0x40u, stack,
           (uint32_t)fd, data_addr, 6u, 0u);
    assert(runtime.cpu.r[0] == 6u);
    assert(memcmp(guest + (data_addr - base), "reside", 6u) == 0);

    /* AAPCS32 lseek: fd in r0, padding in r1, int64 offset in r2/r3,
       whence is the fifth argument and therefore lives at entry SP. */
    guest[stack - base] = 0u;
    guest[stack - base + 1u] = 0u;
    guest[stack - base + 2u] = 0u;
    guest[stack - base + 3u] = 0u;
    invoke(&runtime, exports[2].value, base + 0x60u, stack,
           (uint32_t)fd, 0u, 2u, 0u);
    assert(runtime.cpu.r[0] == 2u);
    invoke(&runtime, exports[1].value, base + 0x80u, stack,
           (uint32_t)fd, data_addr, 4u, 0u);
    assert(runtime.cpu.r[0] == 4u);
    assert(memcmp(guest + (data_addr - base), "side", 4u) == 0);

    invoke(&runtime, exports[4].value, base + 0xa0u, stack,
           path_addr, RE_GUEST_VFS_POSIX_R_OK, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u);
    invoke(&runtime, exports[4].value, base + 0xc0u, stack,
           path_addr, RE_GUEST_VFS_POSIX_W_OK, 0u, 0u);
    assert(runtime.cpu.r[0] == (uint32_t)-1);
    assert(provider.last_errno == RE_GUEST_VFS_POSIX_ERRNO_EACCES);
    assert(guest[errno_addr - base] == RE_GUEST_VFS_POSIX_ERRNO_EACCES);

    invoke(&runtime, exports[1].value, base + 0xe0u, stack,
           (uint32_t)fd, UINT32_MAX, 4u, 0u);
    assert(runtime.cpu.r[0] == (uint32_t)-1);
    assert(provider.last_errno == RE_GUEST_VFS_POSIX_ERRNO_EFAULT);

    invoke(&runtime, exports[3].value, base + 0x100u, stack,
           (uint32_t)fd, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u);
    invoke(&runtime, exports[3].value, base + 0x120u, stack,
           (uint32_t)fd, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == (uint32_t)-1);
    assert(provider.last_errno == RE_GUEST_VFS_POSIX_ERRNO_EBADF);

    puts("Guest VFS POSIX AAPCS32 import tests: PASS");
    return 0;
}
