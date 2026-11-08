#include "re_dtrz_lzma.h"

#include <lzma.h>
#include <string.h>

void re_dtrz_lzma_context_init(re_dtrz_lzma_context *context, uint64_t memlimit) {
    if (!context) return;
    context->memlimit = memlimit ? memlimit : RE_DTRZ_LZMA_DEFAULT_MEMLIMIT;
}

re_dtrz_status re_dtrz_lzma_alone_decode(
    void *opaque, uint32_t method,
    const uint8_t *src, size_t src_size,
    uint8_t *dst, size_t dst_capacity, size_t *written) {
    re_dtrz_lzma_context *context = (re_dtrz_lzma_context *)opaque;
    lzma_stream stream = LZMA_STREAM_INIT;
    uint8_t overflow_probe = 0;
    uint64_t memlimit = context && context->memlimit
        ? context->memlimit : RE_DTRZ_LZMA_DEFAULT_MEMLIMIT;
    lzma_ret result;
    re_dtrz_status status = RE_DTRZ_E_DECODE;

    if (written) *written = 0;
    if (method != RE_DTRZ_METHOD_LZMA_ALONE) return RE_DTRZ_E_UNSUPPORTED;
    if (!src || src_size < 13u || (!dst && dst_capacity != 0u) || !written)
        return RE_DTRZ_E_ARGUMENT;
    if (dst_capacity == 0u) return RE_DTRZ_E_BOUNDS;

    result = lzma_alone_decoder(&stream, memlimit);
    if (result == LZMA_MEM_ERROR) return RE_DTRZ_E_MEMORY;
    if (result != LZMA_OK) return RE_DTRZ_E_DECODE;

    stream.next_in = src;
    stream.avail_in = src_size;
    stream.next_out = dst;
    stream.avail_out = dst_capacity;

    for (;;) {
        result = lzma_code(&stream, LZMA_FINISH);
        if (stream.total_out > dst_capacity) {
            status = RE_DTRZ_E_BOUNDS;
            break;
        }
        if (result == LZMA_STREAM_END) {
            if (stream.total_out > dst_capacity || stream.avail_in != 0u) {
                status = RE_DTRZ_E_DECODE;
            } else {
                *written = (size_t)stream.total_out;
                status = RE_DTRZ_OK;
            }
            break;
        }
        if (result == LZMA_MEMLIMIT_ERROR || result == LZMA_MEM_ERROR) {
            status = RE_DTRZ_E_MEMORY;
            break;
        }
        if (result != LZMA_OK && result != LZMA_BUF_ERROR) {
            status = RE_DTRZ_E_DECODE;
            break;
        }

        /* Once the caller's output buffer is full, permit one extra byte only
         * as an overflow detector. Never expose or retain that byte. */
        if (stream.avail_out == 0u && stream.next_out != &overflow_probe) {
            stream.next_out = &overflow_probe;
            stream.avail_out = 1u;
            continue;
        }
        if (stream.next_out == &overflow_probe && stream.total_out > dst_capacity) {
            status = RE_DTRZ_E_BOUNDS;
            break;
        }
        if (result == LZMA_BUF_ERROR ||
            (stream.avail_in == 0u && result != LZMA_STREAM_END)) {
            status = RE_DTRZ_E_DECODE;
            break;
        }
    }

    lzma_end(&stream);
    if (status != RE_DTRZ_OK) *written = 0;
    return status;
}
