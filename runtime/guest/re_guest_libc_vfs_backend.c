#include "re_guest_libc_vfs_backend.h"

#include <limits.h>
#include <string.h>

static void *vfs_open(void *user, const char *path, const char *mode) {
    REGUESTLIBCVFSBackend *backend = (REGUESTLIBCVFSBackend *)user;
    if (!backend || !backend->initialized || !path || !mode ||
        (strcmp(mode, "r") != 0 && strcmp(mode, "rb") != 0))
        return NULL;
    for (size_t i = 0; i < RE_GUEST_LIBC_VFS_MAX_STREAMS; ++i) {
        REGUESTLIBCVFSStream *slot = &backend->streams[i];
        if (slot->active) continue;
        memset(slot, 0, sizeof(*slot));
        if (re_dtrz_open(backend->vfs, path, &slot->file) != RE_DTRZ_OK)
            return NULL;
        slot->active = 1u;
        return slot;
    }
    return NULL;
}

static int64_t vfs_read(void *user, void *stream, void *dst, uint64_t bytes) {
    REGUESTLIBCVFSBackend *backend = (REGUESTLIBCVFSBackend *)user;
    REGUESTLIBCVFSStream *slot = (REGUESTLIBCVFSStream *)stream;
    size_t amount = 0u;
    if (!backend || !backend->initialized || !slot || !slot->active ||
        (!dst && bytes) || (uint64_t)(size_t)bytes != bytes)
        return -1;
    if (re_dtrz_read(&slot->file, dst, (size_t)bytes, &amount) != RE_DTRZ_OK)
        return -1;
    return (int64_t)amount;
}

static int64_t vfs_write(void *user, void *stream, const void *src, uint64_t bytes) {\n    (void)user; (void)stream; (void)src; (void)bytes;\n    return -1; /* Archive assets are intentionally read-only. */\n}\n\nstatic int vfs_seek(void *user, void *stream, int64_t offset, int whence) {
    REGUESTLIBCVFSBackend *backend = (REGUESTLIBCVFSBackend *)user;
    REGUESTLIBCVFSStream *slot = (REGUESTLIBCVFSStream *)stream;
    uint64_t base, target;
    if (!backend || !backend->initialized || !slot || !slot->active)
        return -1;
    if (whence == 0) base = 0u;
    else if (whence == 1) base = slot->file.cursor;
    else if (whence == 2) base = slot->file.size;
    else return -1;
    if (offset < 0) {
        uint64_t magnitude = (uint64_t)(-(offset + 1)) + 1u;
        if (magnitude > base) return -1;
        target = base - magnitude;
    } else {
        if ((uint64_t)offset > UINT64_MAX - base) return -1;
        target = base + (uint64_t)offset;
    }
    if (target > slot->file.size ||
        re_dtrz_seek(&slot->file, target) != RE_DTRZ_OK)
        return -1;
    return 0;
}

static int64_t vfs_tell(void *user, void *stream) {
    REGUESTLIBCVFSBackend *backend = (REGUESTLIBCVFSBackend *)user;
    REGUESTLIBCVFSStream *slot = (REGUESTLIBCVFSStream *)stream;
    uint64_t offset = 0u;
    if (!backend || !backend->initialized || !slot || !slot->active ||
        re_dtrz_tell(&slot->file, &offset) != RE_DTRZ_OK ||
        offset > INT64_MAX)
        return -1;
    return (int64_t)offset;
}

static int vfs_close(void *user, void *stream) {
    REGUESTLIBCVFSBackend *backend = (REGUESTLIBCVFSBackend *)user;
    REGUESTLIBCVFSStream *slot = (REGUESTLIBCVFSStream *)stream;
    if (!backend || !backend->initialized || !slot || !slot->active)
        return -1;
    re_dtrz_close(&slot->file);
    memset(slot, 0, sizeof(*slot));
    return 0;
}

REGUESTLIBCVFSStatus re_guest_libc_vfs_backend_init(
    REGUESTLIBCVFSBackend *backend, const re_dtrz_vfs *vfs) {
    if (!backend || !vfs || !vfs->archive || !vfs->archive->entries ||
        !vfs->archive->bytes)
        return RE_GUEST_LIBC_VFS_E_ARGUMENT;
    memset(backend, 0, sizeof(*backend));
    backend->vfs = vfs;
    backend->initialized = 1u;
    return RE_GUEST_LIBC_VFS_OK;
}

void re_guest_libc_vfs_backend_get_stdio(
    REGUESTLIBCVFSBackend *backend, REGUESTLIBCStdioBackend *stdio_backend) {
    if (!stdio_backend) return;
    memset(stdio_backend, 0, sizeof(*stdio_backend));
    if (!backend || !backend->initialized) return;
    stdio_backend->open = vfs_open;
    stdio_backend->read = vfs_read;\n    stdio_backend->write = vfs_write;
    stdio_backend->seek = vfs_seek;
    stdio_backend->tell = vfs_tell;
    stdio_backend->close = vfs_close;
}

void re_guest_libc_vfs_backend_close_all(REGUESTLIBCVFSBackend *backend) {
    if (!backend || !backend->initialized) return;
    for (size_t i = 0; i < RE_GUEST_LIBC_VFS_MAX_STREAMS; ++i) {
        REGUESTLIBCVFSStream *slot = &backend->streams[i];
        if (slot->active) {
            re_dtrz_close(&slot->file);
            memset(slot, 0, sizeof(*slot));
        }
    }
}
