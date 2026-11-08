#ifndef S3E_AAPCS32_H
#define S3E_AAPCS32_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    S3E_AAPCS32_OK = 0,
    S3E_AAPCS32_E_ARGUMENT = -1,
    S3E_AAPCS32_E_STACK = -2,
    S3E_AAPCS32_E_ALIGNMENT = -3
} S3EAAPCS32Status;

/* Guest-entry snapshot. stack_words points to guest memory at entry SP.
 * It is not a host pointer into native process memory.
 */
typedef struct {
    uint32_t r[4];
    uint32_t sp;
    const uint8_t *stack_words;
    size_t stack_bytes;
} S3EAAPCS32Frame;

/* Fetch one 32-bit core argument by zero-based AAPCS32 argument-word index.
 * Words 0..3 come from r0..r3; later words come from [entry_sp + 4*(i-4)].
 * This primitive does not infer C parameter alignment or aggregate layout.
 */
S3EAAPCS32Status s3e_aapcs32_arg_word(const S3EAAPCS32Frame *frame,
                                      uint32_t word_index, uint32_t *out);

/* Fetch a 64-bit value from an explicitly supplied even word index.
 * The caller must derive the correct core-register/stack allocation from
 * the function prototype; this helper does not allocate arguments.
 */
S3EAAPCS32Status s3e_aapcs32_arg_u64_even(const S3EAAPCS32Frame *frame,
                                         uint32_t even_word_index, uint64_t *out);

#ifdef __cplusplus
}
#endif
#endif
