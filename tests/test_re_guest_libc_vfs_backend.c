#include "re_guest_libc_vfs_backend.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static const uint8_t payload[] = "game asset data";
    re_dtrz_entry entry;
    re_dtrz_archive archive;
    re_dtrz_vfs vfs;
    REGUESTLIBCVFSBackend owner;
    REGUESTLIBCStdioBackend backend;
    void *stream;
    char output[16] = {0};

    memset(&entry, 0, sizeof(entry));
    entry.path = (char *)"data/config.bin";
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
    assert(re_guest_libc_vfs_backend_init(&owner, &vfs) ==
           RE_GUEST_LIBC_VFS_OK);
    re_guest_libc_vfs_backend_get_stdio(&owner, &backend);
    assert(backend.open && backend.read && backend.seek && backend.tell &&
           backend.close && backend.write && !backend.flush);

    stream = backend.open(&owner, "data/config.bin", "rb");
    assert(stream != NULL);
    assert(backend.read(&owner, stream, output, 4u) == 4);
    assert(memcmp(output, "game", 4u) == 0);
    assert(backend.tell(&owner, stream) == 4);
    assert(backend.seek(&owner, stream, -2, 1) == 0);
    assert(backend.tell(&owner, stream) == 2);
    memset(output, 0, sizeof(output));
    assert(backend.read(&owner, stream, output, sizeof(payload)) ==
           (int64_t)(sizeof(payload) - 1u - 2u));
    assert(memcmp(output, "me asset data", 13u) == 0);
    assert(backend.seek(&owner, stream, 0, 2) == 0);
    assert(backend.tell(&owner, stream) == (int64_t)(sizeof(payload) - 1u));
    assert(backend.seek(&owner, stream, 1, 0) == -1);
    assert(backend.seek(&owner, stream, 0, 99) == -1);
    assert(backend.close(&owner, stream) == 0);
    assert(backend.close(&owner, stream) == -1);

    assert(backend.open(&owner, "data/config.bin", "w") == NULL);
    assert(backend.open(&owner, "../config.bin", "r") == NULL);
    assert(backend.open(&owner, "/data/config.bin", "r") == NULL);
    assert(backend.open(&owner, "missing.bin", "r") == NULL);

    stream = backend.open(&owner, "data/config.bin", "r");
    assert(stream != NULL);
    re_guest_libc_vfs_backend_close_all(&owner);
    assert(backend.tell(&owner, stream) == -1);
    puts("Guest libc VFS stdio backend tests: PASS");
    return 0;
}
