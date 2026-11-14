#ifndef RE_GUEST_LIBC_VFS_BACKEND_H
#define RE_GUEST_LIBC_VFS_BACKEND_H

#include "re_guest_libc_stdio_provider.h"
#include "re_dtrz_vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_LIBC_VFS_MAX_STREAMS RE_GUEST_LIBC_STDIO_MAX_FILES

typedef struct {
    re_dtrz_file file;
    uint8_t active;
} REGUESTLIBCVFSStream;

typedef struct {
    const re_dtrz_vfs *vfs;
    uint8_t initialized;
    REGUESTLIBCVFSStream streams[RE_GUEST_LIBC_VFS_MAX_STREAMS];
} REGUESTLIBCVFSBackend;

typedef enum {
    RE_GUEST_LIBC_VFS_OK = 0,
    RE_GUEST_LIBC_VFS_E_ARGUMENT = -1120,
    RE_GUEST_LIBC_VFS_E_CAPACITY = -1121
} REGUESTLIBCVFSStatus;

/* Read-only stdio backend for archive/VFS assets. Only r/rb modes are accepted.
 * Returned stream pointers refer to caller-owned slots, never guest addresses.
 */
REGUESTLIBCVFSStatus re_guest_libc_vfs_backend_init(
    REGUESTLIBCVFSBackend *backend, const re_dtrz_vfs *vfs);
void re_guest_libc_vfs_backend_get_stdio(
    REGUESTLIBCVFSBackend *backend, REGUESTLIBCStdioBackend *stdio_backend);
void re_guest_libc_vfs_backend_close_all(REGUESTLIBCVFSBackend *backend);

#ifdef __cplusplus
}
#endif
#endif
