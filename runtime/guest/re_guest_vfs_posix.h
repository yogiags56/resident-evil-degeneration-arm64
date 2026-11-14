#ifndef RE_GUEST_VFS_POSIX_H
#define RE_GUEST_VFS_POSIX_H

#include <stddef.h>
#include <stdint.h>
#include "re_dtrz_vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_VFS_POSIX_MAX_FDS 32u
#define RE_GUEST_VFS_POSIX_FD_BASE 3
#define RE_GUEST_VFS_POSIX_O_RDONLY 0u
#define RE_GUEST_VFS_POSIX_O_WRONLY 1u
#define RE_GUEST_VFS_POSIX_O_RDWR 2u
#define RE_GUEST_VFS_POSIX_O_CREAT 0x40u
#define RE_GUEST_VFS_POSIX_O_TRUNC 0x200u
#define RE_GUEST_VFS_POSIX_O_APPEND 0x400u
#define RE_GUEST_VFS_POSIX_F_OK 0u
#define RE_GUEST_VFS_POSIX_X_OK 1u
#define RE_GUEST_VFS_POSIX_W_OK 2u
#define RE_GUEST_VFS_POSIX_R_OK 4u
#define RE_GUEST_VFS_POSIX_S_IFREG 0100000u
#define RE_GUEST_VFS_POSIX_S_IFDIR 0040000u

typedef struct {
    uint32_t mode;
    uint64_t size;
} REGUESTVFSPOSIXStat;

typedef struct {
    re_dtrz_file file;
    int active;
    int descriptor;
    char path[RE_DTRZ_PATH_CAPACITY];
} REGUESTVFSPOSIXSlot;

typedef struct {
    const re_dtrz_vfs *vfs;
    uint8_t initialized;
    REGUESTVFSPOSIXSlot slots[RE_GUEST_VFS_POSIX_MAX_FDS];
} REGUESTVFSPOSIX;

typedef enum {
    RE_GUEST_VFS_POSIX_OK = 0,
    RE_GUEST_VFS_POSIX_E_ARGUMENT = -1140,
    RE_GUEST_VFS_POSIX_E_NOT_FOUND = -1141,
    RE_GUEST_VFS_POSIX_E_ACCESS = -1142,
    RE_GUEST_VFS_POSIX_E_CAPACITY = -1143,
    RE_GUEST_VFS_POSIX_E_DESCRIPTOR = -1144,
    RE_GUEST_VFS_POSIX_E_RANGE = -1145,
    RE_GUEST_VFS_POSIX_E_UNSUPPORTED = -1146
} REGUESTVFSPOSIXStatus;

REGUESTVFSPOSIXStatus re_guest_vfs_posix_init(
    REGUESTVFSPOSIX *fs, const re_dtrz_vfs *vfs);
int re_guest_vfs_posix_open(REGUESTVFSPOSIX *fs, const char *path,
                            uint32_t flags);
int64_t re_guest_vfs_posix_read(REGUESTVFSPOSIX *fs, int fd,
                                void *dst, uint64_t bytes);
int64_t re_guest_vfs_posix_lseek(REGUESTVFSPOSIX *fs, int fd,
                                 int64_t offset, int whence);
int re_guest_vfs_posix_close(REGUESTVFSPOSIX *fs, int fd);
int re_guest_vfs_posix_access(REGUESTVFSPOSIX *fs, const char *path,
                              int mode);
int re_guest_vfs_posix_stat(REGUESTVFSPOSIX *fs, const char *path,
                            REGUESTVFSPOSIXStat *out);
void re_guest_vfs_posix_close_all(REGUESTVFSPOSIX *fs);

#ifdef __cplusplus
}
#endif
#endif
