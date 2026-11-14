#include "re_guest_vfs_posix.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static const uint8_t payload[] = "resident evil asset";
    re_dtrz_entry entry;
    re_dtrz_archive archive;
    re_dtrz_vfs vfs;
    REGUESTVFSPOSIX fs;
    REGUESTVFSPOSIXStat st;
    char buf[32] = {0};
    int fd, fd2;

    memset(&entry, 0, sizeof(entry));
    entry.path = (char *)"data/game.bin";
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
    assert(re_guest_vfs_posix_init(&fs, &vfs) == RE_GUEST_VFS_POSIX_OK);

    assert(re_guest_vfs_posix_access(&fs, "data/game.bin", RE_GUEST_VFS_POSIX_F_OK) == 0);
    assert(re_guest_vfs_posix_access(&fs, "data/game.bin", RE_GUEST_VFS_POSIX_R_OK) == 0);
    assert(re_guest_vfs_posix_access(&fs, "data/game.bin", RE_GUEST_VFS_POSIX_W_OK) == -1);
    assert(re_guest_vfs_posix_access(&fs, "../game.bin", RE_GUEST_VFS_POSIX_F_OK) == -1);
    assert(re_guest_vfs_posix_stat(&fs, "data/game.bin", &st) == 0);
    assert(st.mode == (RE_GUEST_VFS_POSIX_S_IFREG | 0444u));
    assert(st.size == sizeof(payload) - 1u);
    assert(re_guest_vfs_posix_stat(&fs, "missing", &st) == -1);

    assert(re_guest_vfs_posix_open(&fs, "data/game.bin", RE_GUEST_VFS_POSIX_O_RDONLY) == 3);
    fd = 3;
    assert(re_guest_vfs_posix_read(&fs, fd, buf, 4u) == 4);
    assert(memcmp(buf, "resi", 4u) == 0);
    assert(re_guest_vfs_posix_lseek(&fs, fd, -2, 1) == 2);
    assert(re_guest_vfs_posix_lseek(&fs, fd, 0, 0) == 0);
    assert(re_guest_vfs_posix_lseek(&fs, fd, 0, 2) == (int64_t)(sizeof(payload) - 1u));
    assert(re_guest_vfs_posix_read(&fs, fd, buf, sizeof(buf)) == 0);
    assert(re_guest_vfs_posix_lseek(&fs, fd, -5, 2) == (int64_t)(sizeof(payload) - 1u - 5u));
    memset(buf, 0, sizeof(buf));
    assert(re_guest_vfs_posix_read(&fs, fd, buf, 5u) == 5);
    assert(memcmp(buf, "asset", 5u) == 0);
    assert(re_guest_vfs_posix_lseek(&fs, fd, 999, 0) == -1);
    assert(re_guest_vfs_posix_lseek(&fs, fd, 0, 99) == -1);
    assert(re_guest_vfs_posix_close(&fs, fd) == 0);
    assert(re_guest_vfs_posix_close(&fs, fd) == -1);
    assert(re_guest_vfs_posix_read(&fs, fd, buf, 1u) == -1);

    assert(re_guest_vfs_posix_open(&fs, "data/game.bin", RE_GUEST_VFS_POSIX_O_WRONLY) == RE_GUEST_VFS_POSIX_E_ACCESS);
    assert(re_guest_vfs_posix_open(&fs, "data/game.bin", RE_GUEST_VFS_POSIX_O_CREAT) == RE_GUEST_VFS_POSIX_E_ACCESS);
    assert(re_guest_vfs_posix_open(&fs, "../game.bin", RE_GUEST_VFS_POSIX_O_RDONLY) == RE_GUEST_VFS_POSIX_E_NOT_FOUND);
    fd2 = re_guest_vfs_posix_open(&fs, "data/game.bin", RE_GUEST_VFS_POSIX_O_RDONLY);
    assert(fd2 == 3);
    re_guest_vfs_posix_close_all(&fs);
    assert(re_guest_vfs_posix_lseek(&fs, fd2, 0, 0) == -1);
    puts("Guest VFS POSIX descriptor tests: PASS");
    return 0;
}
