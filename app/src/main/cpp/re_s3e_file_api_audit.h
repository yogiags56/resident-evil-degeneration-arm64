#ifndef RE_S3E_FILE_API_AUDIT_H
#define RE_S3E_FILE_API_AUDIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Stage 95: evidence-only capture of the proven AAPCS32 argument words.
 * These functions do not call the backend, access guest memory, or assign
 * guest return registers. They are deliberately not import handlers.
 */
typedef struct {
    uint32_t path_guest_address;
    uint32_t mode_guest_address;
} RES3EFileOpenArgs;

typedef struct {
    uint32_t file_handle;
} RES3EFileHandleArgs;

typedef enum {
    RE_S3E_FILE_API_OK = 0,
    RE_S3E_FILE_API_E_ARGUMENT = -1,
    RE_S3E_FILE_API_E_NULL_PATH = -2,
    RE_S3E_FILE_API_E_NULL_MODE = -3,
    RE_S3E_FILE_API_E_NULL_HANDLE = -4
} RES3EFileApiAuditStatus;

RES3EFileApiAuditStatus re_s3e_file_open_capture(
    const uint32_t r[4], RES3EFileOpenArgs *out);
RES3EFileApiAuditStatus re_s3e_file_get_char_capture(
    const uint32_t r[4], RES3EFileHandleArgs *out);
RES3EFileApiAuditStatus re_s3e_file_close_capture(
    const uint32_t r[4], RES3EFileHandleArgs *out);

/* Always false in Stage 95: no guest file import is authorized by capture
 * evidence alone. The read-return gate remains in Stage 89's module. */
int re_s3e_file_api_guest_imports_authorized(void);

#ifdef __cplusplus
}
#endif
#endif
