#include "app/src/main/cpp/re_s3e_file_read_contract.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void) {
    RES3EFileReadArgs a;
    uint32_t r[4] = {0x1000u, 4u, 1u, 0x44u};
    assert(re_s3e_file_read_capture(r, &a) == RE_S3E_READ_CONTRACT_OK);
    assert(a.destination == 0x1000u && a.item_size == 4u && a.item_count == 1u);
    assert(a.file_handle == 0x44u && a.requested_bytes == 4u);
    r[1] = 2u; r[2] = 16u;
    assert(re_s3e_file_read_capture(r, &a) == RE_S3E_READ_CONTRACT_OK);
    assert(a.requested_bytes == 32u);
    r[1] = 0u;
    assert(re_s3e_file_read_capture(r, &a) == RE_S3E_READ_CONTRACT_ZERO_ITEM_SIZE);
    r[1] = 4u; r[2] = 0u;
    assert(re_s3e_file_read_capture(r, &a) == RE_S3E_READ_CONTRACT_ZERO_COUNT);
    r[2] = 1u; r[0] = 0u;
    assert(re_s3e_file_read_capture(r, &a) == RE_S3E_READ_CONTRACT_NULL_DESTINATION);
    r[0] = 0x1000u; r[3] = 0u;
    assert(re_s3e_file_read_capture(r, &a) == RE_S3E_READ_CONTRACT_NULL_HANDLE);
    assert(re_s3e_file_read_capture(NULL, &a) == RE_S3E_READ_CONTRACT_NULL);
    assert(re_s3e_file_read_capture(r, NULL) == RE_S3E_READ_CONTRACT_NULL);
    r[1] = UINT32_MAX; r[2] = UINT32_MAX; r[3] = 0x44u;
    assert(re_s3e_file_read_capture(r, &a) == RE_S3E_READ_CONTRACT_OK);
    assert(a.requested_bytes == (uint64_t)UINT32_MAX * UINT32_MAX);
    assert(re_s3e_file_read_guest_return_is_authorized() == 0);
    puts("Stage89 contract tests: 10 assertions passed; production read import remains quarantined.");
    return 0;
}
