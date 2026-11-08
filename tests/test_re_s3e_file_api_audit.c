#include "app/src/main/cpp/re_s3e_file_api_audit.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    RES3EFileOpenArgs open_args;
    RES3EFileHandleArgs handle_args;
    uint32_t r[4] = {0x1000u, 0x2000u, 0u, 0u};

    assert(re_s3e_file_open_capture(r, &open_args) == RE_S3E_FILE_API_OK);
    assert(open_args.path_guest_address == 0x1000u);
    assert(open_args.mode_guest_address == 0x2000u);
    r[0] = 0u;
    assert(re_s3e_file_open_capture(r, &open_args) == RE_S3E_FILE_API_E_NULL_PATH);
    r[0] = 0x1000u; r[1] = 0u;
    assert(re_s3e_file_open_capture(r, &open_args) == RE_S3E_FILE_API_E_NULL_MODE);
    assert(re_s3e_file_open_capture(NULL, &open_args) == RE_S3E_FILE_API_E_ARGUMENT);
    assert(re_s3e_file_open_capture(r, NULL) == RE_S3E_FILE_API_E_ARGUMENT);

    r[0] = 0x44u;
    assert(re_s3e_file_get_char_capture(r, &handle_args) == RE_S3E_FILE_API_OK);
    assert(handle_args.file_handle == 0x44u);
    assert(re_s3e_file_close_capture(r, &handle_args) == RE_S3E_FILE_API_OK);
    assert(handle_args.file_handle == 0x44u);
    r[0] = 0u;
    assert(re_s3e_file_get_char_capture(r, &handle_args) == RE_S3E_FILE_API_E_NULL_HANDLE);
    assert(re_s3e_file_close_capture(r, &handle_args) == RE_S3E_FILE_API_E_NULL_HANDLE);
    assert(re_s3e_file_api_guest_imports_authorized() == 0);

    puts("Stage 95 file API argument audit: 14 assertions passed; imports remain disabled.");
    return 0;
}
