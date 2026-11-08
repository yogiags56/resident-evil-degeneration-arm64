#ifndef RE_DTRZ_FILE_BACKEND_H
#define RE_DTRZ_FILE_BACKEND_H

#include "re_dtrz_vfs.h"
#include "re_file_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read-only REFileBackend backed by an already parsed DTRZ archive.
 * Archive bytes, parsed index, VFS and decoder context are borrowed and must
 * outlive this backend and all streams opened through it. */
typedef struct re_dtrz_file_backend {
    const re_dtrz_vfs *vfs;
    uint32_t active_streams;
    re_dtrz_status last_open_status; /* diagnostic: adapter API maps NULL to NOT_FOUND */
} re_dtrz_file_backend;

REFileStatus re_dtrz_file_backend_init(
    re_dtrz_file_backend *backend, const re_dtrz_vfs *vfs,
    REFileBackend *out_callbacks);
uint32_t re_dtrz_file_backend_active_streams(
    const re_dtrz_file_backend *backend);
re_dtrz_status re_dtrz_file_backend_last_open_status(
    const re_dtrz_file_backend *backend);

#ifdef __cplusplus
}
#endif
#endif
