#include "re_dtrz_file_backend.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void put16(unsigned char *p, unsigned v) {
    p[0] = (unsigned char)(v & 0xffu); p[1] = (unsigned char)((v >> 8) & 0xffu);
}
static void put32(unsigned char *p, unsigned v) {
    p[0] = (unsigned char)(v & 0xffu); p[1] = (unsigned char)((v >> 8) & 0xffu);
    p[2] = (unsigned char)((v >> 16) & 0xffu); p[3] = (unsigned char)((v >> 24) & 0xffu);
}
static void put_guest_string(unsigned char *mem, size_t offset, const char *s) {
    memcpy(mem + offset, s, strlen(s) + 1u);
}

int main(void) {
    /* Minimal observed-profile DTRZ fixture: one stored file named sample.bin. */
    unsigned char archive_bytes[52] = {0};
    unsigned char guest_bytes[256] = {0};
    REGUESTMEM guest = { guest_bytes, sizeof(guest_bytes), 0x50000000u };
    re_dtrz_archive archive;
    re_dtrz_vfs vfs;
    re_dtrz_file_backend backend_context;
    REFileBackend callbacks;
    REFileAdapter adapter;
    uint32_t handle = 0;
    uint64_t got = 0;
    int32_t ch = -99;

    memcpy(archive_bytes, "DTRZ", 4);
    put16(archive_bytes + 4, 1); /* file count */
    put16(archive_bytes + 6, 1); /* root folder only */
    archive_bytes[8] = 0;
    memcpy(archive_bytes + 9, "sample.bin", 11);
    put16(archive_bytes + 20, 0); /* folder index */
    put16(archive_bytes + 22, 0); /* file index */
    put16(archive_bytes + 24, 0); /* flags */
    put16(archive_bytes + 26, 0); /* location header opaque */
    put16(archive_bytes + 28, 1); /* location record count */
    put32(archive_bytes + 30, 46); /* first payload offset */
    put32(archive_bytes + 34, 6);  /* decoded size A */
    put32(archive_bytes + 38, 6);  /* decoded size B */
    put32(archive_bytes + 42, RE_DTRZ_METHOD_STORED);
    memcpy(archive_bytes + 46, "REDGEN", 6);

    assert(re_dtrz_archive_parse(&archive, archive_bytes, sizeof(archive_bytes)) == RE_DTRZ_OK);
    assert(re_dtrz_vfs_init(&vfs, &archive, NULL, NULL) == RE_DTRZ_OK);
    assert(re_dtrz_file_backend_init(&backend_context, &vfs, &callbacks) == RE_FILE_OK);
    assert(re_file_adapter_init(&adapter, &callbacks, &backend_context) == RE_FILE_OK);

    put_guest_string(guest_bytes, 0x20, "sample.bin");
    put_guest_string(guest_bytes, 0x50, "rb");
    assert(re_file_adapter_open(&adapter, &guest, 0x50000020u, 0x50000050u, &handle) == RE_FILE_OK);
    assert(handle != 0);
    assert(re_dtrz_file_backend_active_streams(&backend_context) == 1);
    assert(re_dtrz_file_backend_last_open_status(&backend_context) == RE_DTRZ_OK);

    /* Read is sequential and partial reads are valid at EOF. */
    assert(re_file_adapter_read_bytes(&adapter, &guest, handle, 0x50000080u, 4, &got) == RE_FILE_OK);
    assert(got == 4 && memcmp(guest_bytes + 0x80, "REDG", 4) == 0);
    assert(re_file_adapter_get_char(&adapter, handle, &ch) == RE_FILE_OK && ch == 'E');
    assert(re_file_adapter_read_bytes(&adapter, &guest, handle, 0x50000090u, 8, &got) == RE_FILE_OK);
    assert(got == 1 && guest_bytes[0x90] == 'N');
    assert(re_file_adapter_get_char(&adapter, handle, &ch) == RE_FILE_OK && ch == -1);

    assert(re_file_adapter_close(&adapter, handle) == RE_FILE_OK);
    assert(re_dtrz_file_backend_active_streams(&backend_context) == 0);
    assert(re_file_adapter_get_char(&adapter, handle, &ch) == RE_FILE_E_BAD_HANDLE);

    /* Missing path fails closed; backend retains the precise VFS diagnostic. */
    put_guest_string(guest_bytes, 0x20, "absent.bin");
    assert(re_file_adapter_open(&adapter, &guest, 0x50000020u, 0x50000050u, &handle) == RE_FILE_E_NOT_FOUND);
    assert(re_dtrz_file_backend_last_open_status(&backend_context) == RE_DTRZ_E_NOT_FOUND);

    /* Traversal is rejected before backend access. */
    put_guest_string(guest_bytes, 0x20, "../sample.bin");
    assert(re_file_adapter_open(&adapter, &guest, 0x50000020u, 0x50000050u, &handle) == RE_FILE_E_PATH);
    assert(re_dtrz_file_backend_active_streams(&backend_context) == 0);

    re_file_adapter_close_all(&adapter);
    re_dtrz_archive_destroy(&archive);
    puts("Stage 94 DTRZ file backend integration tests passed (20 assertions)");
    return 0;
}
