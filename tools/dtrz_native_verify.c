#include "re_dtrz_vfs.h"
#include "re_dtrz_lzma.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static unsigned long long hash64(const unsigned char *p, size_t n) {
    /* FNV-1a 64-bit: diagnostic fingerprint, not a cryptographic hash. */
    unsigned long long h = 14695981039346656037ull;
    size_t i;
    for (i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}

static unsigned char *read_all(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    long end;
    unsigned char *data;
    if (!f || fseek(f, 0, SEEK_END) != 0 || (end = ftell(f)) < 0 ||
        fseek(f, 0, SEEK_SET) != 0) { if (f) fclose(f); return NULL; }
    data = (unsigned char *)malloc((size_t)end ? (size_t)end : 1u);
    if (!data || fread(data, 1, (size_t)end, f) != (size_t)end) {
        free(data); fclose(f); return NULL;
    }
    fclose(f); *size = (size_t)end; return data;
}

static int write_entry(const char *root, const char *relative,
                       const unsigned char *data, size_t size) {
    char path[4096];
    size_t root_len = strlen(root), rel_len = strlen(relative), i;
    FILE *f;
    if (root_len + 1u + rel_len + 1u > sizeof(path)) return 0;
    if (mkdir(root, 0755) != 0 && errno != EEXIST) return 0;
    memcpy(path, root, root_len);
    if (root_len && root[root_len - 1u] != '/') path[root_len++] = '/';
    memcpy(path + root_len, relative, rel_len + 1u);
    for (i = root_len; i < root_len + rel_len; ++i) {
        if (path[i] == '/') {
            char saved = path[i]; path[i] = '\0';
            if (mkdir(path, 0755) != 0 && errno != EEXIST) return 0;
            path[i] = saved;
        }
    }
    f = fopen(path, "wb");
    if (!f) return 0;
    {
        size_t wrote = fwrite(data, 1, size, f);
        int close_result = fclose(f);
        if (wrote != size || close_result != 0) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    unsigned char *bytes;
    size_t size, i;
    re_dtrz_archive archive;
    re_dtrz_vfs vfs;
    re_dtrz_lzma_context codec;
    unsigned long long aggregate = 0;
    if (argc != 3) { fprintf(stderr, "usage: %s main.dz output-directory\n", argv[0]); return 2; }
    bytes = read_all(argv[1], &size);
    if (!bytes) { fprintf(stderr, "cannot read archive\n"); return 2; }
    if (re_dtrz_archive_parse(&archive, bytes, size) != RE_DTRZ_OK) {
        fprintf(stderr, "DTRZ parse failed\n"); free(bytes); return 1;
    }
    re_dtrz_lzma_context_init(&codec, RE_DTRZ_LZMA_DEFAULT_MEMLIMIT);
    re_dtrz_vfs_init(&vfs, &archive, re_dtrz_lzma_alone_decode, &codec);
    for (i = 0; i < archive.file_count; ++i) {
        re_dtrz_file file;
        const re_dtrz_entry *entry = &archive.entries[i];
        unsigned long long h;
        if (re_dtrz_open(&vfs, entry->path, &file) != RE_DTRZ_OK) {
            fprintf(stderr, "decode failed: %s (method=0x%x packed=%llu decoded=%u)\n",
                entry->path, entry->method,
                (unsigned long long)entry->packed_size, entry->decoded_size_a);
            re_dtrz_archive_destroy(&archive); free(bytes); return 1;
        }
        h = hash64(file.decoded, file.size);
        aggregate ^= h + 0x9e3779b97f4a7c15ull + (aggregate << 6) + (aggregate >> 2);
        if (!write_entry(argv[2], entry->path, file.decoded, file.size)) {
            fprintf(stderr, "write failed: %s\n", entry->path);
            re_dtrz_close(&file); re_dtrz_archive_destroy(&archive); free(bytes); return 1;
        }
        printf("OK\t%u\t%s\t%zu\t%016llx\n", entry->index, entry->path,
            file.size, h);
        re_dtrz_close(&file);
    }
    printf("SUMMARY\t%u entries\taggregate=%016llx\n", archive.file_count, aggregate);
    re_dtrz_archive_destroy(&archive); free(bytes); return 0;
}
