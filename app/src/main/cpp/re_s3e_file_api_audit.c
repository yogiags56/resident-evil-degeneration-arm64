#include "re_s3e_file_api_audit.h"

int re_s3e_file_api_guest_imports_authorized(void) { return 0; }

RES3EFileApiAuditStatus re_s3e_file_open_capture(
    const uint32_t r[4], RES3EFileOpenArgs *out) {
    if (!r || !out) return RE_S3E_FILE_API_E_ARGUMENT;
    out->path_guest_address = r[0];
    out->mode_guest_address = r[1];
    if (r[0] == 0u) return RE_S3E_FILE_API_E_NULL_PATH;
    if (r[1] == 0u) return RE_S3E_FILE_API_E_NULL_MODE;
    return RE_S3E_FILE_API_OK;
}

RES3EFileApiAuditStatus re_s3e_file_get_char_capture(
    const uint32_t r[4], RES3EFileHandleArgs *out) {
    if (!r || !out) return RE_S3E_FILE_API_E_ARGUMENT;
    out->file_handle = r[0];
    return r[0] ? RE_S3E_FILE_API_OK : RE_S3E_FILE_API_E_NULL_HANDLE;
}

RES3EFileApiAuditStatus re_s3e_file_close_capture(
    const uint32_t r[4], RES3EFileHandleArgs *out) {
    if (!r || !out) return RE_S3E_FILE_API_E_ARGUMENT;
    out->file_handle = r[0];
    return r[0] ? RE_S3E_FILE_API_OK : RE_S3E_FILE_API_E_NULL_HANDLE;
}
