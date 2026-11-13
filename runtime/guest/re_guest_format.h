#ifndef RE_GUEST_FORMAT_H
#define RE_GUEST_FORMAT_H

#include <stddef.h>
#include <stdint.h>

#include "s3e_aapcs32.h"
#include "re_guest_memory_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_FORMAT_STRING_CAP 2048u
#define RE_GUEST_FORMAT_OUTPUT_CAP 2048u

/* Bounded AAPCS32 variadic formatter. first_word_index is the zero-based
 * core-register/stack word index of the first variadic argument.
 * Returns zero on unsupported format, bad guest memory, or output overflow.
 */
int re_guest_format_guest_printf(const S3EAAPCS32Frame *frame,
                                 const REGUESTMEMBUS *bus,
                                 uint32_t first_word_index,
                                 const char *format,
                                 char *output, size_t output_capacity,
                                 size_t *output_length);

#ifdef __cplusplus
}
#endif
#endif
