#include "re_s3e_file_bridge.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    const uint8_t *bytes;
    size_t length;
    size_t cursor;
    uint32_t opens;
    uint32_t closes;
    uint32_t reads;
    uint32_t chars;
    int missing;
    int read_error;
    int char_error;
} MockBackend;

typedef struct {
    MockBackend *owner;
    size_t cursor;
} MockStream;

static MockStream streams[8];

static void *mock_open(void *opaque, const char *path, const char *mode) {
    MockBackend *backend = (MockBackend *)opaque;
    if (!backend || backend->missing || strcmp(path, "data/test.bin") != 0 ||
        strcmp(mode, "rb") != 0) return NULL;
    for (size_t i = 0; i < sizeof(streams) / sizeof(streams[0]); ++i) {
        if (streams[i].owner == NULL) {
            backend->opens++;
            streams[i].owner = backend;
            streams[i].cursor = 0;
            return &streams[i];
        }
    }
    return NULL;
}

static int64_t mock_read(void *opaque, void *opaque_stream,
                         void *destination, uint64_t requested) {
    MockBackend *backend = (MockBackend *)opaque;
    MockStream *s = (MockStream *)opaque_stream;
    size_t remaining, count;
    if (!backend || !s || s->owner != backend || (!destination && requested) || backend->read_error)
        return -1;
    backend->reads++;
    remaining = s->cursor < backend->length
        ? backend->length - s->cursor : 0;
    count = requested < remaining ? (size_t)requested : remaining;
    if (count) memcpy(destination, backend->bytes + s->cursor, count);
    s->cursor += count;
    return (int64_t)count;
}

static int32_t mock_get_char(void *opaque, void *opaque_stream) {
    MockBackend *backend = (MockBackend *)opaque;
    MockStream *s = (MockStream *)opaque_stream;
    if (!backend || !s || s->owner != backend || backend->char_error) return -2;
    backend->chars++;
    if (s->cursor >= backend->length) return -1;
    return backend->bytes[s->cursor++];
}

static void mock_close(void *opaque, void *opaque_stream) {
    MockBackend *backend = (MockBackend *)opaque;
    MockStream *s = (MockStream *)opaque_stream;
    if (backend && s && s->owner == backend) {
        backend->closes++;
        s->owner = NULL;
    }
}

static void put_string(uint8_t *memory, size_t offset, const char *value) {
    memcpy(memory + offset, value, strlen(value) + 1u);
}

int main(void) {
    static const uint8_t file_bytes[] = { 'A','B','C','D','E','F','G' };
    uint8_t guest_bytes[512] = {0};
    REGUESTMEM memory = { guest_bytes, sizeof(guest_bytes), 0x1000u };
    REFileBackend callbacks = { mock_open, mock_read, mock_get_char, mock_close };
    MockBackend backend = { file_bytes, sizeof(file_bytes), 0, 0, 0, 0, 0, 0, 0, 0 };
    REFileAdapter adapter;
    RES3EFileBridge bridge;
    uint32_t args[4] = {0}, handle = 0, items = 0;
    int32_t character = -99;
    uint32_t assertions = 0;

#define CHECK(x) do { assert(x); assertions++; } while (0)

    put_string(guest_bytes, 0, "data/test.bin");
    put_string(guest_bytes, 32, "rb");
    CHECK(re_file_adapter_init(&adapter, &callbacks, &backend) == RE_FILE_OK);
    CHECK(re_s3e_file_bridge_init(&bridge, &adapter) == RE_S3E_BRIDGE_OK);

    args[0] = 0x1000u; args[1] = 0x1020u;
    CHECK(re_s3e_file_bridge_open(&bridge, &memory, args, &handle) == RE_S3E_BRIDGE_OK);
    CHECK(handle != 0u && backend.opens == 1u);

    /* Three 2-byte items: exactly six bytes copied and three items returned. */
    args[0] = 0x1040u; args[1] = 2u; args[2] = 3u; args[3] = handle;
    CHECK(re_s3e_file_bridge_read(&bridge, &memory, args, &items) == RE_S3E_BRIDGE_OK);
    CHECK(items == 3u);
    CHECK(memcmp(guest_bytes + 64, "ABCDEF", 6) == 0);

    /* One requested 2-byte item with only one byte left: byte copied,
       but no complete item is counted. This mirrors fread item-count rules. */
    args[0] = 0x1050u; args[1] = 2u; args[2] = 1u; args[3] = handle;
    CHECK(re_s3e_file_bridge_read(&bridge, &memory, args, &items) == RE_S3E_BRIDGE_OK);
    CHECK(items == 0u);
    CHECK(guest_bytes[80] == 'G');

    /* EOF is a successful read of zero complete items. */
    args[0] = 0x1051u; args[1] = 1u; args[2] = 1u; args[3] = handle;
    CHECK(re_s3e_file_bridge_read(&bridge, &memory, args, &items) == RE_S3E_BRIDGE_OK);
    CHECK(items == 0u);

    CHECK(re_s3e_file_bridge_get_char(&bridge, (uint32_t[4]){handle,0,0,0},
                                      &character) == RE_S3E_BRIDGE_OK);
    CHECK(character == -1);

    /* Bad and stale tokens never reach the backend. */
    CHECK(re_s3e_file_bridge_get_char(&bridge, (uint32_t[4]){0xDEADBEEFu,0,0,0},
                                      &character) == RE_S3E_BRIDGE_E_BAD_HANDLE);
    CHECK(re_s3e_file_bridge_close(&bridge, (uint32_t[4]){0xDEADBEEFu,0,0,0})
          == RE_S3E_BRIDGE_E_BAD_HANDLE);

    CHECK(re_s3e_file_bridge_close(&bridge, (uint32_t[4]){handle,0,0,0})
          == RE_S3E_BRIDGE_OK);
    CHECK(backend.closes == 1u);
    CHECK(re_s3e_file_bridge_close(&bridge, (uint32_t[4]){handle,0,0,0})
          == RE_S3E_BRIDGE_E_BAD_HANDLE);

    /* A reopened stream receives a new generation token. */
    args[0] = 0x1000u; args[1] = 0x1020u;
    CHECK(re_s3e_file_bridge_open(&bridge, &memory, args, &handle) == RE_S3E_BRIDGE_OK);
    CHECK(handle != 0u);
    CHECK(re_s3e_file_bridge_get_char(&bridge, (uint32_t[4]){handle,0,0,0},
                                      &character) == RE_S3E_BRIDGE_OK);
    CHECK(character == 'A');
    CHECK(re_s3e_file_bridge_close(&bridge, (uint32_t[4]){handle,0,0,0})
          == RE_S3E_BRIDGE_OK);

    /* Missing file maps to a host status; no guessed guest error word. */
    backend.missing = 1;
    CHECK(re_s3e_file_bridge_open(&bridge, &memory,
                                  (uint32_t[4]){0x1000u,0x1020u,0,0},
                                  &handle) == RE_S3E_BRIDGE_E_OPEN);
    CHECK(handle == 0u);

    /* Traversal and invalid guest spans are rejected before backend access. */
    put_string(guest_bytes, 0, "../escape.bin");
    backend.missing = 0;
    CHECK(re_s3e_file_bridge_open(&bridge, &memory,
                                  (uint32_t[4]){0x1000u,0x1020u,0,0},
                                  &handle) == RE_S3E_BRIDGE_E_OPEN);
    put_string(guest_bytes, 0, "data/test.bin");
    CHECK(re_s3e_file_bridge_read(&bridge, &memory,
                                  (uint32_t[4]){0x2000u,1u,1u,0x12345678u},
                                  &items) == RE_S3E_BRIDGE_E_BAD_HANDLE);
    CHECK(items == 0u);

    /* A valid token with an out-of-range destination is rejected. */
    CHECK(re_s3e_file_bridge_open(&bridge, &memory,
                                  (uint32_t[4]){0x1000u,0x1020u,0,0},
                                  &handle) == RE_S3E_BRIDGE_OK);
    CHECK(re_s3e_file_bridge_read(&bridge, &memory,
                                  (uint32_t[4]){0x11FFu,1u,2u,handle},
                                  &items) == RE_S3E_BRIDGE_E_READ);
    CHECK(items == 0u);
    backend.read_error = 1;
    CHECK(re_s3e_file_bridge_read(&bridge, &memory,
                                  (uint32_t[4]){0x1060u,1u,1u,handle},
                                  &items) == RE_S3E_BRIDGE_E_READ);
    CHECK(items == 0u);
    backend.read_error = 0;
    backend.char_error = 1;
    CHECK(re_s3e_file_bridge_get_char(&bridge, (uint32_t[4]){handle,0,0,0},
                                      &character) == RE_S3E_BRIDGE_E_GET_CHAR);
    backend.char_error = 0;
    CHECK(re_s3e_file_bridge_close(&bridge, (uint32_t[4]){handle,0,0,0})
          == RE_S3E_BRIDGE_OK);

    CHECK(re_s3e_file_bridge_open(&bridge, &memory,
                                  (uint32_t[4]){0x1000u,0x1020u,0,0},
                                  &handle) == RE_S3E_BRIDGE_OK);

    /* Handler wrappers expose only proven successful values. Open failure
       returns the null handle expected by the original caller's null check. */
    {
        S3EAAPCS32Frame frame = {{0x1000u,0x1020u,0,0}, 0x1000u,
                                 guest_bytes, sizeof(guest_bytes)};
        REGUESTIMPORTResult result = {0xFFFFFFFFu, 0xFFFFFFFFu, 0u, 0u};
        backend.missing = 1;
        CHECK(re_s3e_file_bridge_open_handler(&frame, &memory, &bridge,
                                               &result) == RE_IMPORT_OK);
        CHECK(result.r0 == 0u);
        backend.missing = 0;
        CHECK(re_s3e_file_bridge_open_handler(&frame, &memory, &bridge,
                                               &result) == RE_IMPORT_OK);
        handle = result.r0;
        CHECK(handle != 0u);
        frame.r[0] = 0x1060u; frame.r[1] = 1u; frame.r[2] = 2u; frame.r[3] = handle;
        CHECK(re_s3e_file_bridge_read_handler(&frame, &memory, &bridge,
                                               &result) == RE_IMPORT_OK);
        CHECK(result.r0 == 2u);
        frame.r[0] = handle;
        CHECK(re_s3e_file_bridge_get_char_handler(&frame, &memory, &bridge,
                                                   &result) == RE_IMPORT_OK);
        CHECK(result.r0 == (uint32_t)'C');
        CHECK(re_s3e_file_bridge_close_handler(&frame, &memory, &bridge,
                                                &result) == RE_IMPORT_OK);
    }

    CHECK(re_s3e_file_bridge_production_imports_authorized() == 0);
    re_file_adapter_close_all(&adapter);
    CHECK(backend.closes == 5u);

    printf("Stage 98 guest file bridge: PASS (%u assertions)\n", assertions);
    return 0;
#undef CHECK
}
