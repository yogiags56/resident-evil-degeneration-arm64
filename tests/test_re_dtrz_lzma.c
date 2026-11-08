#include "re_dtrz_lzma.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    static const unsigned char packed[] = {
        0x5d,0x00,0x00,0x80,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0x00,0x34,0x19,0x49,0xee,0x8d,0xe9,0x15,0x12,0x26,0x4c,0xb0,0xef,
        0x64,0x3d,0xf0,0x25,0xcd,0x88,0x2f,0xa4,0x48,0xff,0xff,0x1e,0xb4,0x00,0x00
    };
    const char expected[] = "hello native lzma";
    unsigned char output[sizeof(expected)] = {0};
    size_t written = 0;
    re_dtrz_lzma_context context;

    re_dtrz_lzma_context_init(&context, RE_DTRZ_LZMA_DEFAULT_MEMLIMIT);
    assert(re_dtrz_lzma_alone_decode(&context, RE_DTRZ_METHOD_LZMA_ALONE,
        packed, sizeof(packed), output, sizeof(output), &written) == RE_DTRZ_OK);
    assert(written == sizeof(expected) - 1u);
    assert(memcmp(output, expected, written) == 0);

    written = 123;
    assert(re_dtrz_lzma_alone_decode(&context, RE_DTRZ_METHOD_LZMA_ALONE,
        packed, sizeof(packed), output, 4u, &written) == RE_DTRZ_E_BOUNDS);
    assert(written == 0u);

    written = 123;
    assert(re_dtrz_lzma_alone_decode(&context, RE_DTRZ_METHOD_STORED,
        packed, sizeof(packed), output, sizeof(output), &written) == RE_DTRZ_E_UNSUPPORTED);
    assert(written == 0u);

    written = 123;
    assert(re_dtrz_lzma_alone_decode(&context, RE_DTRZ_METHOD_LZMA_ALONE,
        packed, 8u, output, sizeof(output), &written) == RE_DTRZ_E_ARGUMENT);
    assert(written == 0u);

    re_dtrz_lzma_context_init(&context, 1024u * 1024u);
    written = 123;
    assert(re_dtrz_lzma_alone_decode(&context, RE_DTRZ_METHOD_LZMA_ALONE,
        packed, sizeof(packed), output, sizeof(output), &written) == RE_DTRZ_E_MEMORY);
    assert(written == 0u);

    {
        unsigned char corrupt[sizeof(packed)];
        memcpy(corrupt, packed, sizeof(packed));
        corrupt[14] ^= 0xffu;
        re_dtrz_lzma_context_init(&context, RE_DTRZ_LZMA_DEFAULT_MEMLIMIT);
        written = 123;
        assert(re_dtrz_lzma_alone_decode(&context, RE_DTRZ_METHOD_LZMA_ALONE,
            corrupt, sizeof(corrupt), output, sizeof(output), &written) == RE_DTRZ_E_DECODE);
        assert(written == 0u);
    }

    puts("test_re_dtrz_lzma: 6 scenarios passed");
    return 0;
}
