#include "re_guest_vfs_posix.h"

#include <limits.h>
#include <string.h>

static REGUESTVFSPOSIXSlot *find_fd(REGUESTVFSPOSIX *fs, int fd) {
    if (!fs || !fs->initialized || fd < RE_GUEST_VFS_POSIX_FD_BASE)
        return NULL;
    for (size_t i = 0; i < RE_GUEST_VFS_POSIX_MAX_FDS; ++i) {
        REGUESTVFSPOSIXSlot *slot = &fs->slots[i];
        if (slot->active && slot->descriptor == fd) return slot;
    }
    return NULL;
}

REGUESTVFSPOSIXStatus re_guest_vfs_posix_init(
    REGUESTVFSPOSIX *fs, const re_dtrz_vfs *vfs) {
    if (!fs || !vfs || !vfs->archive || !vfs->archive->entries ||
        !vfs->archive->bytes)
        return RE_GUEST_VFS_POSIX_E_ARGUMENT;
    memset(fs, 0, sizeof(*fs));
    fs->vfs = vfs;
    fs->initialized = 1u;
    return RE_GUEST_VFS_POSIX_OK;
}

int re_guest_vfs_posix_open(REGUESTVFSPOSIX *fs, const char *path,
                            uint32_t flags) {
    const uint32_t allowed = RE_GUEST_VFS_POSIX_O_RDONLY;
    if (!fs || !fs->initialized || !path) return RE_GUEST_VFS_POSIX_E_ARGUMENT;
    if (flags != allowed) return RE_GUEST_VFS_POSIX_E_ACCESS;
    if (!re_dtrz_find(fs->vfs->archive, path)) return RE_GUEST_VFS_POSIX_E_NOT_FOUND;
    for (size_t i = 0; i < RE_GUEST_VFS_POSIX_MAX_FDS; ++i) {
        REGUESTVFSPOSIXSlot *slot = &fs->slots[i];
        if (slot->active) continue;
        memset(slot, 0, sizeof(*slot));
        if (re_dtrz_open(fs->vfs, path, &slot->file) != RE_DTRZ_OK)
            return RE_GUEST_VFS_POSIX_E_NOT_FOUND;
        size_t n = strlen(re_dtrz_find(fs->vfs->archive, path)->path);
        if (n >= sizeof(slot->path)) {
            re_dtrz_close(&slot->file);
            return RE_GUEST_VFS_POSIX_E_RANGE;
        }
        memcpy(slot->path, re_dtrz_find(fs->vfs->archive, path)->path, n + 1u);
        slot->descriptor = RE_GUEST_VFS_POSIX_FD_BASE + (int)i;
        slot->active = 1;
        return slot->descriptor;
    }
    return RE_GUEST_VFS_POSIX_E_CAPACITY;
}

int64_t re_guest_vfs_posix_read(REGUESTVFSPOSIX *fs, int fd,
                                void *dst, uint64_t bytes) {
    REGUESTVFSPOSIXSlot *slot = find_fd(fs, fd);
    size_t amount = 0u;
    if (!slot || (!dst && bytes) || (uint64_t)(size_t)bytes != bytes)
        return -1;
    if (re_dtrz_read(&slot->file, dst, (size_t)bytes, &amount) != RE_DTRZ_OK)
        return -1;
    return (int64_t)amount;
}

int64_t re_guest_vfs_posix_lseek(REGUESTVFSPOSIX *fs, int fd,
                                 int64_t offset, int whence) {
    REGUESTVFSPOSIXSlot *slot = find_fd(fs, fd);
    uint64_t base, target;
    if (!slot) return -1;
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
    if (target > slot->file.size || target > INT64_MAX ||
        re_dtrz_seek(&slot->file, target) != RE_DTRZ_OK)
        return -1;
    return (int64_t)target;
}

int re_guest_vfs_posix_close(REGUESTVFSPOSIX *fs, int fd) {
    REGUESTVFSPOSIXSlot *slot = find_fd(fs, fd);
    if (!slot) return -1;
    re_dtrz_close(&slot->file);
    memset(slot, 0, sizeof(*slot));
    return 0;
}

int re_guest_vfs_posix_access(REGUESTVFSPOSIX *fs, const char *path,
                              int mode) {
    if (!fs || !fs->initialized || !path || mode < 0 ||
        (mode & ~(RE_GUEST_VFS_POSIX_R_OK | RE_GUEST_VFS_POSIX_W_OK |
                  RE_GUEST_VFS_POSIX_X_OK)))
        return -1;
    if (mode & (RE_GUEST_VFS_POSIX_W_OK | RE_GUEST_VFS_POSIX_X_OK))
        return -1;
    return re_dtrz_find(fs->vfs->archive, path) ? 0 : -1;
}

int re_guest_vfs_posix_stat(REGUESTVFSPOSIX *fs, const char *path,
                            REGUESTVFSPOSIXStat *out) {
    const re_dtrz_entry *entry;
    if (!fs || !fs->initialized || !path || !out) return -1;
    entry = re_dtrz_find(fs->vfs->archive, path);
    if (!entry) return -1;
    out->mode = RE_GUEST_VFS_POSIX_S_IFREG | 0444u;
    out->size = entry->decoded_size_a;
    return 0;
}

void re_guest_vfs_posix_close_all(REGUESTVFSPOSIX *fs) {
    if (!fs || !fs->initialized) return;
    for (size_t i = 0; i < RE_GUEST_VFS_POSIX_MAX_FDS; ++i) {
        if (fs->slots[i].active) {
            re_dtrz_close(&fs->slots[i].file);
            memset(&fs->slots[i], 0, sizeof(fs->slots[i]));
        }
    }
}
