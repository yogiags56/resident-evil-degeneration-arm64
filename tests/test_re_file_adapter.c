\
#include "re_file_adapter.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    const unsigned char *data;
    size_t size;
} MockAsset;

typedef struct {
    const MockAsset *assets;
    size_t asset_count;
    unsigned opens, closes;
    const char *last_path;
} MockFS;

typedef struct {
    const MockAsset *asset;
    size_t position;
} MockStream;

static void *mock_open(void *user, const char *path, const char *mode) {
    MockFS *fs = (MockFS *)user;
    size_t i;
    (void)mode;
    fs->opens++;
    fs->last_path = path;
    for (i = 0; i < fs->asset_count; ++i) {
        if (strcmp(fs->assets[i].name, path) == 0) {
            MockStream *s = (MockStream *)malloc(sizeof(*s));
            if (!s) return NULL;
            s->asset = &fs->assets[i];
            s->position = 0;
            return s;
        }
    }
    return NULL;
}

static int64_t mock_read(void *user, void *stream, void *dst, uint64_t count) {
    MockStream *s = (MockStream *)stream;
    size_t remaining, take;
    (void)user;
    remaining = s->asset->size - s->position;
    take = count < remaining ? (size_t)count : remaining;
    memcpy(dst, s->asset->data + s->position, take);
    s->position += take;
    return (int64_t)take;
}

static int32_t mock_get_char(void *user, void *stream) {
    MockStream *s = (MockStream *)stream;
    (void)user;
    if (s->position >= s->asset->size) return -1;
    return s->asset->data[s->position++];
}

static void mock_close(void *user, void *stream) {
    MockFS *fs = (MockFS *)user;
    fs->closes++;
    free(stream);
}

static void put_string(unsigned char *mem, size_t offset, const char *s) {
    memcpy(mem + offset, s, strlen(s) + 1u);
}

int main(void) {
    static const unsigned char payload[] = { 'R','E','D','G','E','N' };
    static const MockAsset assets[] = { { "assets/sample.bin", payload, sizeof(payload) } };
    unsigned char guest_bytes[512] = {0};
    REGUESTMEM memory = { guest_bytes, sizeof(guest_bytes), 0x50000000u };
    MockFS fs = { assets, 1, 0, 0, NULL };
    REFileBackend backend = { mock_open, mock_read, mock_get_char, mock_close };
    REFileAdapter adapter;
    uint32_t handle = 0, handle2 = 0;
    uint64_t bytes = 0;
    int32_t ch = -99;
    uint8_t *span = NULL;
    char normalized[RE_FILE_ADAPTER_MAX_PATH];

    assert(re_file_adapter_init(&adapter, &backend, &fs) == RE_FILE_OK);

    /* Guest virtual address translation and boundary enforcement. */
    assert(re_file_guest_span(&memory, 0x50000010u, 8, &span) == RE_FILE_OK);
    assert(span == guest_bytes + 16);
    assert(re_file_guest_span(&memory, 0x4fffffffu, 1, &span) == RE_FILE_E_GUEST_MEMORY);
    assert(re_file_guest_span(&memory, 0x500001ffu, 2, &span) == RE_FILE_E_GUEST_MEMORY);
    assert(re_file_guest_span(&memory, 0x50000010u, 0, &span) == RE_FILE_OK);

    /* Path policy: safe relative path accepted, traversal/absolute rejected. */
    assert(re_file_normalize_path("assets//./sample.bin", normalized) == RE_FILE_OK);
    assert(strcmp(normalized, "assets/sample.bin") == 0);
    assert(re_file_normalize_path("../secret", normalized) == RE_FILE_E_PATH);
    assert(re_file_normalize_path("/absolute", normalized) == RE_FILE_E_PATH);
    assert(re_file_normalize_path("C:/secret", normalized) == RE_FILE_E_PATH);
    assert(re_file_normalize_path(".", normalized) == RE_FILE_E_PATH);

    /* Strings reside in guest memory; only observed rb mode is accepted. */
    put_string(guest_bytes, 0x20, "assets//sample.bin");
    put_string(guest_bytes, 0x80, "rb");
    assert(re_file_adapter_open(&adapter, &memory, 0x50000020u, 0x50000080u, &handle) == RE_FILE_OK);
    assert(handle != 0 && fs.opens == 1);
    assert(strcmp(fs.last_path, "assets/sample.bin") == 0);

    /* Read bytes into a validated guest span; partial read at EOF is valid. */
    assert(re_file_adapter_read_bytes(&adapter, &memory, handle, 0x50000100u, 4, &bytes) == RE_FILE_OK);
    assert(bytes == 4 && memcmp(guest_bytes + 0x100, "REDG", 4) == 0);
    assert(re_file_adapter_read_bytes(&adapter, &memory, handle, 0x50000110u, 8, &bytes) == RE_FILE_OK);
    assert(bytes == 2 && memcmp(guest_bytes + 0x110, "EN", 2) == 0);
    assert(re_file_adapter_get_char(&adapter, handle, &ch) == RE_FILE_OK && ch == -1);

    /* Invalid guest destination must not reach backend. */
    assert(re_file_adapter_read_bytes(&adapter, &memory, handle, 0x500001ffu, 4, &bytes) == RE_FILE_E_GUEST_MEMORY);
    assert(bytes == 0);

    /* Stale handle rejected after close; a reused slot gets a new token. */
    assert(re_file_adapter_close(&adapter, handle) == RE_FILE_OK);
    assert(fs.closes == 1);
    assert(re_file_adapter_get_char(&adapter, handle, &ch) == RE_FILE_E_BAD_HANDLE);
    assert(re_file_adapter_open(&adapter, &memory, 0x50000020u, 0x50000080u, &handle2) == RE_FILE_OK);
    assert(handle2 != handle);
    re_file_adapter_close_all(&adapter);
    assert(fs.closes == 2);
    assert(re_file_adapter_close(&adapter, handle2) == RE_FILE_E_BAD_HANDLE);

    /* Handle table capacity is bounded and close-all releases every live stream. */
    {
        uint32_t many[RE_FILE_ADAPTER_MAX_HANDLES];
        uint32_t extra = 0;
        size_t i;
        for (i = 0; i < RE_FILE_ADAPTER_MAX_HANDLES; ++i) {
            assert(re_file_adapter_open(&adapter, &memory, 0x50000020u,
                                        0x50000080u, &many[i]) == RE_FILE_OK);
        }
        assert(re_file_adapter_open(&adapter, &memory, 0x50000020u,
                                    0x50000080u, &extra) == RE_FILE_E_CAPACITY);
        re_file_adapter_close_all(&adapter);
        assert(fs.closes == 2u + RE_FILE_ADAPTER_MAX_HANDLES);
    }

    /* Missing asset, invalid mode and invalid guest strings fail closed. */
    put_string(guest_bytes, 0x20, "assets/missing.bin");
    assert(re_file_adapter_open(&adapter, &memory, 0x50000020u, 0x50000080u, &handle) == RE_FILE_E_NOT_FOUND);
    put_string(guest_bytes, 0x80, "r+");
    assert(re_file_adapter_open(&adapter, &memory, 0x50000020u, 0x50000080u, &handle) == RE_FILE_E_MODE);
    assert(re_file_adapter_open(&adapter, &memory, 0x50000200u, 0x50000080u, &handle) == RE_FILE_E_GUEST_MEMORY);

    puts("Stage 82 file adapter tests passed");
    return 0;
}
