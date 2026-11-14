#ifndef RE_GUEST_VFS_POSIX_IMPORTS_H
#define RE_GUEST_VFS_POSIX_IMPORTS_H

#include "re_guest_imports.h"
#include "re_guest_vfs_posix.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT 5u
#define RE_GUEST_VFS_POSIX_IMPORT_MAX_PATH 512u

typedef enum {
    RE_GUEST_VFS_POSIX_ERRNO_NONE = 0,
    RE_GUEST_VFS_POSIX_ERRNO_EFAULT = 14,
    RE_GUEST_VFS_POSIX_ERRNO_EBADF = 9,
    RE_GUEST_VFS_POSIX_ERRNO_EACCES = 13,
    RE_GUEST_VFS_POSIX_ERRNO_ENOENT = 2,
    RE_GUEST_VFS_POSIX_ERRNO_EMFILE = 24,
    RE_GUEST_VFS_POSIX_ERRNO_EINVAL = 22,
    RE_GUEST_VFS_POSIX_ERRNO_EIO = 5
} REGUESTVFSPOSIXErrno;

typedef struct {
    uint32_t kind;
    struct REGUESTVFSPOSIXImportProvider *provider;
} REGUESTVFSPOSIXImportContext;

typedef struct REGUESTVFSPOSIXImportProvider {
    REGUESTVFSPOSIX *filesystem;
    uint32_t guest_errno_address; /* Optional guest int; zero disables writes. */
    int last_errno;
    uint8_t initialized;
    REGUESTVFSPOSIXImportContext contexts[RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT];
} REGUESTVFSPOSIXImportProvider;

typedef enum {
    RE_GUEST_VFS_POSIX_IMPORT_OK = 0,
    RE_GUEST_VFS_POSIX_IMPORT_E_ARGUMENT = -1150,
    RE_GUEST_VFS_POSIX_IMPORT_E_SYMBOL = -1151,
    RE_GUEST_VFS_POSIX_IMPORT_E_CAPACITY = -1152,
    RE_GUEST_VFS_POSIX_IMPORT_E_BIND = -1153
} REGUESTVFSPOSIXImportStatus;

REGUESTVFSPOSIXImportStatus re_guest_vfs_posix_import_provider_init(
    REGUESTVFSPOSIXImportProvider *provider, REGUESTVFSPOSIX *filesystem,
    uint32_t guest_errno_address);
REGUESTVFSPOSIXImportStatus re_guest_vfs_posix_import_provider_bind_elf_symbols(
    REGUESTVFSPOSIXImportProvider *provider, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity, size_t *export_count,
    size_t *failed_symbol_index);

#ifdef __cplusplus
}
#endif
#endif
