#ifndef RE_S3E_FILE_READ_CONTRACT_H
#define RE_S3E_FILE_READ_CONTRACT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* AAPCS32 argument capture only. This is NOT an enabled guest import. */
typedef struct {
    uint32_t destination;
    uint32_t item_size;
    uint32_t item_count;
    uint32_t file_handle;
    uint64_t requested_bytes;
} RES3EFileReadArgs;
typedef enum {
    RE_S3E_READ_CONTRACT_OK = 0,
    RE_S3E_READ_CONTRACT_NULL = -1,
    RE_S3E_READ_CONTRACT_ZERO_ITEM_SIZE = -2,
    RE_S3E_READ_CONTRACT_ZERO_COUNT = -3,
    RE_S3E_READ_CONTRACT_SIZE_OVERFLOW = -4,
    RE_S3E_READ_CONTRACT_NULL_DESTINATION = -5,
    RE_S3E_READ_CONTRACT_NULL_HANDLE = -6
} RES3EFileReadContractStatus;
/* Captures r0-r3 and validates arithmetic/obvious nulls. Does not read memory,
 * call the backend, or assign a guest r0 return value. */
RES3EFileReadContractStatus re_s3e_file_read_capture(
    const uint32_t r[4], RES3EFileReadArgs *out);
/* Always false until exact partial-read and error semantics are proven; Stage 97 establishes the return unit as items. */
int re_s3e_file_read_guest_return_is_authorized(void);
#ifdef __cplusplus
}
#endif
#endif
