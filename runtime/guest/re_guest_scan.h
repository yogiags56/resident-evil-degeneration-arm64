#ifndef RE_GUEST_SCAN_H
#define RE_GUEST_SCAN_H

#include <stddef.h>
#include <stdint.h>

#include "s3e_aapcs32.h"
#include "re_guest_memory_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_SCAN_FORMAT_CAP 2048u
#define RE_GUEST_SCAN_TOKEN_CAP 256u

/* Bounded scanf-family parser. Variadic arguments are guest destination
 * addresses fetched as AAPCS32 words. No host scanf receives guest pointers.
 * Returns 1 for a valid format/execution, 0 for invalid format or memory fault.
 * A normal matching failure is reported through matched=0 and assignments.
 */
int re_guest_scan_guest_sscanf(const S3EAAPCS32Frame *frame,
                               REGUESTMEMBUS *bus,
                               uint32_t first_word_index,
                               const char *input, const char *format,
                               uint32_t *assignments,
                               size_t *input_consumed,
                               int *input_failure);

#ifdef __cplusplus
}
#endif
#endif
