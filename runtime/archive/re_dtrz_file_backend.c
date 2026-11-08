#include "re_dtrz_file_backend.h"

#include <stdlib.h>
#include <string.h>

typedef struct re_dtrz_backend_stream {
    re_dtrz_file file;
} re_dtrz_backend_stream;

static void *backend_open(void *opaque, const char *path, const char *mode) {
    re_dtrz_file_backend *backend = (re_dtrz_file_backend *)opaque;
    re_dtrz_backend_stream *stream;
    if (!backend || !backend->vfs || !path || !mode || strcmp(mode, "rb") != 0)
        return NULL;
    backend->last_open_status = RE_DTRZ_E_ARGUMENT;
    stream = (re_dtrz_backend_stream *)calloc(1u, sizeof(*stream));
    if (!stream) return NULL;
    backend->last_open_status = re_dtrz_open(backend->vfs, path, &stream->file);
    if (backend->last_open_status != RE_DTRZ_OK) {
        free(stream);
        return NULL;
    }
    ++backend->active_streams;
    return stream;
}

static int64_t backend_read(void *opaque, void *opaque_stream,
                            void *destination, uint64_t requested) {
    re_dtrz_backend_stream *stream = (re_dtrz_backend_stream *)opaque_stream;
    size_t read_count = 0;
    re_dtrz_status status;
    (void)opaque;
    if (!stream || (!destination && requested != 0u) || requested > SIZE_MAX ||
        requested > INT64_MAX) return -1;
    status = re_dtrz_read(&stream->file, destination, (size_t)requested, &read_count);
    if (status != RE_DTRZ_OK || read_count > (size_t)INT64_MAX) return -1;
    return (int64_t)read_count;
}

static int32_t backend_get_char(void *opaque, void *opaque_stream) {
    re_dtrz_backend_stream *stream = (re_dtrz_backend_stream *)opaque_stream;
    uint8_t byte = 0;
    size_t read_count = 0;
    re_dtrz_status status;
    (void)opaque;
    if (!stream) return -2;
    status = re_dtrz_read(&stream->file, &byte, 1u, &read_count);
    if (status != RE_DTRZ_OK) return -2;
    return read_count == 0u ? -1 : (int32_t)byte;
}

static void backend_close(void *opaque, void *opaque_stream) {
    re_dtrz_file_backend *backend = (re_dtrz_file_backend *)opaque;
    re_dtrz_backend_stream *stream = (re_dtrz_backend_stream *)opaque_stream;
    if (!stream) return;
    re_dtrz_close(&stream->file);
    if (backend && backend->active_streams > 0u) --backend->active_streams;
    free(stream);
}

REFileStatus re_dtrz_file_backend_init(
    re_dtrz_file_backend *backend, const re_dtrz_vfs *vfs,
    REFileBackend *out_callbacks) {
    if (!backend || !vfs || !vfs->archive || !out_callbacks)
        return RE_FILE_E_ARGUMENT;
    memset(backend, 0, sizeof(*backend));
    backend->vfs = vfs;
    backend->last_open_status = RE_DTRZ_OK;
    out_callbacks->open = backend_open;
    out_callbacks->read = backend_read;
    out_callbacks->get_char = backend_get_char;
    out_callbacks->close = backend_close;
    return RE_FILE_OK;
}

uint32_t re_dtrz_file_backend_active_streams(
    const re_dtrz_file_backend *backend) {
    return backend ? backend->active_streams : 0u;
}

re_dtrz_status re_dtrz_file_backend_last_open_status(
    const re_dtrz_file_backend *backend) {
    return backend ? backend->last_open_status : RE_DTRZ_E_ARGUMENT;
}
