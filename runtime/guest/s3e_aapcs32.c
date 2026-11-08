#include "s3e_aapcs32.h"

static uint32_t load_le32(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

S3EAAPCS32Status s3e_aapcs32_arg_word(const S3EAAPCS32Frame *f,
                                      uint32_t i, uint32_t *out) {
    size_t offset;
    if (!f || !out) return S3E_AAPCS32_E_ARGUMENT;
    if (i < 4u) {
        *out = f->r[i];
        return S3E_AAPCS32_OK;
    }
    offset = (size_t)(i - 4u) * 4u;
    if (!f->stack_words || offset > f->stack_bytes ||
        f->stack_bytes - offset < 4u)
        return S3E_AAPCS32_E_STACK;
    *out = load_le32(f->stack_words + offset);
    return S3E_AAPCS32_OK;
}

S3EAAPCS32Status s3e_aapcs32_arg_u64_even(const S3EAAPCS32Frame *f,
                                         uint32_t i, uint64_t *out) {
    uint32_t lo, hi;
    S3EAAPCS32Status s;
    if (!out) return S3E_AAPCS32_E_ARGUMENT;
    if ((i & 1u) != 0u) return S3E_AAPCS32_E_ALIGNMENT;
    s = s3e_aapcs32_arg_word(f, i, &lo);
    if (s != S3E_AAPCS32_OK) return s;
    s = s3e_aapcs32_arg_word(f, i + 1u, &hi);
    if (s != S3E_AAPCS32_OK) return s;
    *out = ((uint64_t)hi << 32) | lo;
    return S3E_AAPCS32_OK;
}
