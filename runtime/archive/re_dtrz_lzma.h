#ifndef RE_DTRZ_LZMA_H
#define RE_DTRZ_LZMA_H

#include "re_dtrz_vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LZMA-alone decoder backed by liblzma. The memory limit applies to the
 * decoder dictionary/state; output remains bounded by dst_capacity. */
#define RE_DTRZ_LZMA_DEFAULT_MEMLIMIT (64u * 1024u * 1024u)

typedef struct re_dtrz_lzma_context {
    uint64_t memlimit;
} re_dtrz_lzma_context;

void re_dtrz_lzma_context_init(re_dtrz_lzma_context *context, uint64_t memlimit);
re_dtrz_status re_dtrz_lzma_alone_decode(
    void *context, uint32_t method,
    const uint8_t *src, size_t src_size,
    uint8_t *dst, size_t dst_capacity, size_t *written);

#ifdef __cplusplus
}
#endif
#endif
