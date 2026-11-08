#include "re_s3e_file_read_contract.h"
#include <stddef.h>
#include <stdint.h>
int re_s3e_file_read_guest_return_is_authorized(void) { return 0; }
RES3EFileReadContractStatus re_s3e_file_read_capture(
    const uint32_t r[4], RES3EFileReadArgs *out) {
    uint64_t total;
    if (r == NULL || out == NULL) return RE_S3E_READ_CONTRACT_NULL;
    out->destination = r[0];
    out->item_size = r[1];
    out->item_count = r[2];
    out->file_handle = r[3];
    out->requested_bytes = 0;
    if (r[1] == 0) return RE_S3E_READ_CONTRACT_ZERO_ITEM_SIZE;
    if (r[2] == 0) return RE_S3E_READ_CONTRACT_ZERO_COUNT;
    if (r[0] == 0) return RE_S3E_READ_CONTRACT_NULL_DESTINATION;
    if (r[3] == 0) return RE_S3E_READ_CONTRACT_NULL_HANDLE;
    total = (uint64_t)r[1] * (uint64_t)r[2];
    /* Both inputs are uint32, so uint64 product is representable. Keep the
       explicit check as a guard if the ABI is widened in a later revision. */
    if (r[1] != 0 && total / r[1] != r[2])
        return RE_S3E_READ_CONTRACT_SIZE_OVERFLOW;
    out->requested_bytes = total;
    return RE_S3E_READ_CONTRACT_OK;
}
