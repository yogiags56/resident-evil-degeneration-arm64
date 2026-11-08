#include "re_dtrz_file_backend.h"
#include "re_dtrz_lzma.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GUEST_BASE 0x50000000u
#define GUEST_SIZE (16u * 1024u)
#define PATH_OFFSET 0x20u
#define MODE_OFFSET 0x300u
#define DATA_OFFSET 0x1000u
#define CHUNK_SIZE 4096u

static uint32_t fnv1a_update(uint32_t hash, const uint8_t *bytes, size_t size) {
    size_t i;
    for (i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

static int read_file(const char *path, uint8_t **bytes, size_t *size) {
    FILE *f = fopen(path, "rb");
    long length;
    uint8_t *data;
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0 || (length = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    data = (uint8_t *)malloc((size_t)length ? (size_t)length : 1u);
    if (!data) { fclose(f); return 0; }
    if (fread(data, 1u, (size_t)length, f) != (size_t)length) {
        free(data); fclose(f); return 0;
    }
    fclose(f);
    *bytes = data;
    *size = (size_t)length;
    return 1;
}

int main(int argc, char **argv) {
    uint8_t *archive_bytes = NULL;
    size_t archive_size = 0;
    uint8_t guest_bytes[GUEST_SIZE] = {0};
    REGUESTMEM guest = { guest_bytes, sizeof(guest_bytes), GUEST_BASE };
    re_dtrz_archive archive;
    re_dtrz_vfs vfs;
    re_dtrz_lzma_context lzma_context;
    re_dtrz_file_backend backend_context;
    REFileBackend callbacks;
    REFileAdapter adapter;
    uint32_t i;
    uint64_t total_bytes = 0;
    if (argc != 2) {
        fprintf(stderr, "usage: %s /path/to/main.dz\n", argv[0]);
        return 2;
    }
    if (!read_file(argv[1], &archive_bytes, &archive_size)) {
        fprintf(stderr, "cannot read archive: %s\n", argv[1]);
        return 2;
    }
    if (re_dtrz_archive_parse(&archive, archive_bytes, archive_size) != RE_DTRZ_OK) {
        fprintf(stderr, "DTRZ parse failed\n"); free(archive_bytes); return 1;
    }
    re_dtrz_lzma_context_init(&lzma_context, RE_DTRZ_LZMA_DEFAULT_MEMLIMIT);
    if (re_dtrz_vfs_init(&vfs, &archive, re_dtrz_lzma_alone_decode,
                         &lzma_context) != RE_DTRZ_OK ||
        re_dtrz_file_backend_init(&backend_context, &vfs, &callbacks) != RE_FILE_OK ||
        re_file_adapter_init(&adapter, &callbacks, &backend_context) != RE_FILE_OK) {
        fprintf(stderr, "adapter initialization failed\n");
        re_dtrz_archive_destroy(&archive); free(archive_bytes); return 1;
    }
    memcpy(guest_bytes + MODE_OFFSET, "rb", 3u);
    for (i = 0; i < archive.file_count; ++i) {
        const re_dtrz_entry *entry = &archive.entries[i];
        uint32_t handle = 0;
        uint64_t file_total = 0;
        uint32_t hash = 2166136261u;
        REFileStatus fs;
        size_t path_len = strlen(entry->path) + 1u;
        if (path_len > MODE_OFFSET - PATH_OFFSET) {
            fprintf(stderr, "path too long: %s\n", entry->path); goto fail;
        }
        memcpy(guest_bytes + PATH_OFFSET, entry->path, path_len);
        fs = re_file_adapter_open(&adapter, &guest, GUEST_BASE + PATH_OFFSET,
                                  GUEST_BASE + MODE_OFFSET, &handle);
        if (fs != RE_FILE_OK) {
            fprintf(stderr, "open failed for %s: adapter=%d vfs=%d\n", entry->path,
                    (int)fs, (int)re_dtrz_file_backend_last_open_status(&backend_context));
            goto fail;
        }
        for (;;) {
            uint64_t got = 0;
            fs = re_file_adapter_read_bytes(&adapter, &guest, handle,
                    GUEST_BASE + DATA_OFFSET, CHUNK_SIZE, &got);
            if (fs != RE_FILE_OK) {
                fprintf(stderr, "read failed for %s: %d\n", entry->path, (int)fs);
                (void)re_file_adapter_close(&adapter, handle);
                goto fail;
            }
            if (got == 0u) break;
            hash = fnv1a_update(hash, guest_bytes + DATA_OFFSET, (size_t)got);
            file_total += got;
        }
        if (re_file_adapter_close(&adapter, handle) != RE_FILE_OK) {
            fprintf(stderr, "close failed for %s\n", entry->path); goto fail;
        }
        if (file_total != entry->decoded_size_a) {
            fprintf(stderr, "size mismatch for %s: got=%llu expected=%u\n", entry->path,
                    (unsigned long long)file_total, entry->decoded_size_a); goto fail;
        }
        total_bytes += file_total;
        printf("OK\t%s\t%llu\t%08x\n", entry->path,
               (unsigned long long)file_total, hash);
    }
    if (re_dtrz_file_backend_active_streams(&backend_context) != 0u) {
        fprintf(stderr, "stream leak detected\n"); goto fail;
    }
    printf("PASS: %u entries, %llu decoded bytes through REFileAdapter\n",
           archive.file_count, (unsigned long long)total_bytes);
    re_dtrz_archive_destroy(&archive);
    free(archive_bytes);
    return 0;
fail:
    re_file_adapter_close_all(&adapter);
    re_dtrz_archive_destroy(&archive);
    free(archive_bytes);
    return 1;
}
