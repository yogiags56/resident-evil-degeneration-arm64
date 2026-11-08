#include "re_dtrz_vfs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}

static re_dtrz_status fake_lzma(void *context, uint32_t method,
    const uint8_t *src, size_t src_size, uint8_t *dst,
    size_t dst_capacity, size_t *written) {
    (void)context;
    if (method != RE_DTRZ_METHOD_LZMA_ALONE || src_size != 4 ||
        memcmp(src, "PACK", 4) != 0 || dst_capacity < 3) return RE_DTRZ_E_DECODE;
    memcpy(dst, "xyz", 3);
    *written = 3;
    return RE_DTRZ_OK;
}

static uint8_t *make_fixture(size_t *out_size) {
    const size_t payload = 9u + 11u + 10u + 7u + 12u + 4u + 32u;
    const size_t total = payload + 5u + 4u;
    uint8_t *b = (uint8_t *)calloc(total, 1);
    size_t p = 9u;
    assert(b != NULL);
    memcpy(b, "DTRZ", 4); put16(b + 4, 2); put16(b + 6, 2); b[8] = 0;
    memcpy(b + p, "readme.txt", 10); p += 11;
    memcpy(b + p, "asset.bin", 9); p += 10;
    memcpy(b + p, "folder", 6); p += 7;
    assert(p == 37u);
    put16(b + p, 0); put16(b + p + 2, 0); put16(b + p + 4, 0xffff); p += 6;
    put16(b + p, 1); put16(b + p + 2, 1); put16(b + p + 4, 0xffff); p += 6;
    put16(b + p, 1); put16(b + p + 2, 2); p += 4;
    assert(p == payload - 32u);
    put32(b + p, (uint32_t)payload); put32(b + p + 4, 5); put32(b + p + 8, 5);
    put32(b + p + 12, RE_DTRZ_METHOD_STORED); p += 16;
    put32(b + p, (uint32_t)(payload + 5u)); put32(b + p + 4, 3); put32(b + p + 8, 3);
    put32(b + p + 12, RE_DTRZ_METHOD_LZMA_ALONE); p += 16;
    assert(p == payload);
    memcpy(b + p, "hello", 5); memcpy(b + p + 5, "PACK", 4);
    *out_size = total;
    return b;
}

int main(void) {
    uint8_t *bytes;
    size_t byte_count, n;
    re_dtrz_archive archive;
    re_dtrz_vfs vfs;
    re_dtrz_file file;
    const re_dtrz_entry *entry;
    char output[8] = {0};
    uint64_t offset = 0;

    bytes = make_fixture(&byte_count);
    assert(re_dtrz_archive_parse(&archive, bytes, byte_count) == RE_DTRZ_OK);
    assert(archive.file_count == 2);
    assert(archive.folder_count == 2);
    assert(archive.payload_offset == 85u);
    entry = re_dtrz_find(&archive, "readme.txt");
    assert(entry && entry->method == RE_DTRZ_METHOD_STORED && entry->packed_size == 5u);
    entry = re_dtrz_find(&archive, "folder\\asset.bin");
    assert(entry && strcmp(entry->path, "folder/asset.bin") == 0);
    assert(re_dtrz_find(&archive, "../readme.txt") == NULL);
    assert(re_dtrz_find(&archive, "/readme.txt") == NULL);

    assert(re_dtrz_vfs_init(&vfs, &archive, fake_lzma, NULL) == RE_DTRZ_OK);
    assert(re_dtrz_open(&vfs, "readme.txt", &file) == RE_DTRZ_OK);
    assert(re_dtrz_read(&file, output, 2, &n) == RE_DTRZ_OK && n == 2);
    assert(memcmp(output, "he", 2) == 0);
    assert(re_dtrz_tell(&file, &offset) == RE_DTRZ_OK && offset == 2);
    assert(re_dtrz_seek(&file, 4) == RE_DTRZ_OK);
    assert(re_dtrz_read(&file, output, sizeof(output), &n) == RE_DTRZ_OK && n == 1);
    assert(output[0] == 'o');
    assert(re_dtrz_read(&file, output, 1, &n) == RE_DTRZ_OK && n == 0);
    re_dtrz_close(&file);
    assert(re_dtrz_read(&file, output, 1, &n) == RE_DTRZ_E_CLOSED);

    {
        re_dtrz_vfs no_codec;
        assert(re_dtrz_vfs_init(&no_codec, &archive, NULL, NULL) == RE_DTRZ_OK);
        assert(re_dtrz_open(&no_codec, "folder/asset.bin", &file) == RE_DTRZ_E_UNSUPPORTED);
    }
    /* Reject oversized decoded entries before attempting allocation. */
    re_dtrz_archive_destroy(&archive);
    put32(bytes + 73u, 0xffffffffu);
    put32(bytes + 77u, 0xffffffffu);
    assert(re_dtrz_archive_parse(&archive, bytes, byte_count) == RE_DTRZ_OK);
    assert(re_dtrz_vfs_init(&vfs, &archive, fake_lzma, NULL) == RE_DTRZ_OK);
    assert(re_dtrz_open(&vfs, "folder/asset.bin", &file) == RE_DTRZ_E_BOUNDS);
    re_dtrz_archive_destroy(&archive);
    put32(bytes + 73u, 3u);
    put32(bytes + 77u, 3u);
    assert(re_dtrz_archive_parse(&archive, bytes, byte_count) == RE_DTRZ_OK);
    assert(re_dtrz_vfs_init(&vfs, &archive, fake_lzma, NULL) == RE_DTRZ_OK);

    assert(re_dtrz_open(&vfs, "folder/asset.bin", &file) == RE_DTRZ_OK);
    memset(output, 0, sizeof(output));
    assert(re_dtrz_read(&file, output, 3, &n) == RE_DTRZ_OK && n == 3);
    assert(memcmp(output, "xyz", 3) == 0);
    re_dtrz_close(&file);
    assert(re_dtrz_open(&vfs, "missing.bin", &file) == RE_DTRZ_E_NOT_FOUND);

    /* Corrupt the first location offset: parser must reject before indexing. */
    re_dtrz_archive_destroy(&archive);
    put32(bytes + 53u, 1u);
    assert(re_dtrz_archive_parse(&archive, bytes, byte_count) == RE_DTRZ_E_FORMAT);
    free(bytes);
    puts("test_re_dtrz_vfs: 29 assertions passed");
    return 0;
}
