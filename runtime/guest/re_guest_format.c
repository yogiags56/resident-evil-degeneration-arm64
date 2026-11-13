#include "re_guest_format.h"

#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    LOG_LEN_NONE, LOG_LEN_HH, LOG_LEN_H, LOG_LEN_L, LOG_LEN_LL,
    LOG_LEN_Z, LOG_LEN_T, LOG_LEN_J, LOG_LEN_CAP_L
} LogLength;

static int read_guest_string(const REGUESTMEMBUS *bus, uint32_t address,
                             char *out, size_t capacity) {
    if (!bus || !bus->memory || !out || !capacity || !address) return 0;
    for (size_t i = 0; i < capacity; ++i) {
        uint8_t byte;
        uint64_t at = (uint64_t)address + i;
        if (at > UINT32_MAX ||
            re_guest_memory_read8(bus, (uint32_t)at, &byte) != RE_GUEST_MEM_OK)
            return 0;
        if (byte == 0) {
            out[i] = '\0';
            return 1;
        }
        if (byte < 0x20u && byte != '\t') return 0;
        if (byte == 0x7fu) return 0;
        out[i] = (char)byte;
    }
    out[capacity - 1u] = '\0';
    return 0;
}

static int get_word(const S3EAAPCS32Frame *frame, uint32_t index,
                    uint32_t *value) {
    return s3e_aapcs32_arg_word(frame, index, value) == S3E_AAPCS32_OK;
}

static int get_u64_even(const S3EAAPCS32Frame *frame, uint32_t *index,
                        uint64_t *value) {
    uint32_t low, high;
    if (!frame || !index || !value) return 0;
    if (*index & 1u) {
        if (*index == UINT32_MAX) return 0;
        ++*index;
    }
    if (!get_word(frame, *index, &low)) return 0;
    if (*index > UINT32_MAX - 2u ||
        !get_word(frame, *index + 1u, &high)) return 0;
    *index += 2u;
    *value = (uint64_t)low | ((uint64_t)high << 32);
    return 1;
}

static int append_bytes(char *out, size_t capacity, size_t *used,
                        const char *bytes, size_t length) {
    if (!out || !used || (!bytes && length) ||
        *used >= capacity || length > capacity - *used - 1u)
        return 0;
    if (length) memcpy(out + *used, bytes, length);
    *used += length;
    out[*used] = '\0';
    return 1;
}

static int append_snprintf(char *out, size_t capacity, size_t *used,
                           const char *spec, ...) {
    char temp[512];
    int n;
    va_list ap;
    va_start(ap, spec);
    n = vsnprintf(temp, sizeof(temp), spec, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof(temp)) return 0;
    return append_bytes(out, capacity, used, temp, (size_t)n);
}

static int append_number(char *spec, size_t capacity, size_t *used,
                         unsigned value) {
    char digits[16];
    int n = snprintf(digits, sizeof(digits), "%u", value);
    if (n < 0 || (size_t)n >= sizeof(digits) ||
        (size_t)n > capacity - *used)
        return 0;
    memcpy(spec + *used, digits, (size_t)n);
    *used += (size_t)n;
    return 1;
}

int re_guest_format_guest_printf(const S3EAAPCS32Frame *frame,
                                 const REGUESTMEMBUS *bus,
                                 uint32_t first_word_index,
                                 const char *format,
                                 char *output, size_t output_capacity,
                                 size_t *output_length) {
    size_t used = 0;
    uint32_t word_index = first_word_index;
    const char *p = format;
    if (!frame || !bus || !format || !output || !output_capacity) return 0;
    output[0] = '\0';

    while (*p) {
        const char *start = p;
        char spec[80];
        size_t spec_used = 0;
        int width = 0, precision = -1;
        int have_width = 0, have_precision = 0;
        int negative_width = 0;
        LogLength length = LOG_LEN_NONE;
        char conversion;
        uint32_t word = 0;
        uint64_t wide = 0;

        if (*p != '%') {
            while (*p && *p != '%') ++p;
            if (!append_bytes(output, output_capacity, &used, start,
                              (size_t)(p - start))) return 0;
            continue;
        }
        ++p;
        if (*p == '%') {
            if (!append_bytes(output, output_capacity, &used, "%", 1u)) return 0;
            ++p;
            continue;
        }
        if (!*p) return 0;
        spec[spec_used++] = '%';

        while (*p && strchr("-+ #0'", *p)) {
            if (spec_used + 2u >= sizeof(spec)) return 0;
            spec[spec_used++] = *p++;
        }

        if (*p == '*') {
            if (!get_word(frame, word_index++, &word)) return 0;
            width = (int32_t)word;
            have_width = 1;
            ++p;
            if (width < 0) {
                negative_width = 1;
                if (width == INT_MIN) return 0;
                width = -width;
            }
            if ((unsigned)width > 256u) return 0;
            if (negative_width && !memchr(spec, '-', spec_used)) {
                if (spec_used + 1u >= sizeof(spec)) return 0;
                spec[spec_used++] = '-';
            }
            if (!append_number(spec, sizeof(spec), &spec_used,
                               (unsigned)width)) return 0;
        } else {
            while (*p >= '0' && *p <= '9') {
                if (!have_width) have_width = 1;
                if (width > 25) return 0;
                width = width * 10 + (*p++ - '0');
                if (width > 256) return 0;
            }
            if (have_width && !append_number(spec, sizeof(spec),
                    &spec_used, (unsigned)width)) return 0;
        }

        if (*p == '.') {
            ++p;
            have_precision = 1;
            precision = 0;
            if (*p == '*') {
                if (!get_word(frame, word_index++, &word)) return 0;
                precision = (int32_t)word;
                ++p;
                if (precision < 0) {
                    have_precision = 0;
                    precision = -1;
                    /* Negative dynamic precision is treated as omitted. */
                } else if (precision > 256) return 0;
            } else {
                while (*p >= '0' && *p <= '9') {
                    if (precision > 25) return 0;
                    precision = precision * 10 + (*p++ - '0');
                    if (precision > 256) return 0;
                }
            }
            if (have_precision) {
                if (spec_used + 1u >= sizeof(spec)) return 0;
                spec[spec_used++] = '.';
                if (!append_number(spec, sizeof(spec), &spec_used,
                                   (unsigned)precision)) return 0;
            }
        }

        if (*p == 'h') {
            ++p;
            length = LOG_LEN_H;
            if (*p == 'h') { ++p; length = LOG_LEN_HH; }
        } else if (*p == 'l') {
            ++p;
            length = LOG_LEN_L;
            if (*p == 'l') { ++p; length = LOG_LEN_LL; }
        } else if (*p == 'z') {
            ++p; length = LOG_LEN_Z;
        } else if (*p == 't') {
            ++p; length = LOG_LEN_T;
        } else if (*p == 'j') {
            ++p; length = LOG_LEN_J;
        } else if (*p == 'L') {
            ++p; length = LOG_LEN_CAP_L;
        }
        if (*p < 0x21 || *p > 0x7e) return 0;
        conversion = *p++;
        if (strchr("diuoxXcsp", conversion) ||
            strchr("fFeEgGaA", conversion)) {
            if (length == LOG_LEN_CAP_L || length == LOG_LEN_Z ||
                length == LOG_LEN_T || length == LOG_LEN_J) return 0;
        } else {
            return 0; /* Reject %n, positional args, and unknown extensions. */
        }
        if (length != LOG_LEN_NONE) {
            const char *len_text = NULL;
            switch (length) {
                case LOG_LEN_HH: len_text = "hh"; break;
                case LOG_LEN_H: len_text = "h"; break;
                case LOG_LEN_L: len_text = "l"; break;
                case LOG_LEN_LL: len_text = "ll"; break;
                case LOG_LEN_Z: len_text = "z"; break;
                case LOG_LEN_T: len_text = "t"; break;
                case LOG_LEN_J: len_text = "j"; break;
                default: return 0;
            }
            if (spec_used + strlen(len_text) + 2u >= sizeof(spec)) return 0;
            memcpy(spec + spec_used, len_text, strlen(len_text));
            spec_used += strlen(len_text);
        }
        if (spec_used + 2u > sizeof(spec)) return 0;
        spec[spec_used++] = conversion;
        spec[spec_used] = '\0';

        if (conversion == 's') {
            char guest_string[RE_GUEST_FORMAT_STRING_CAP];
            if (length != LOG_LEN_NONE || !get_word(frame, word_index++, &word) ||
                !read_guest_string(bus, word, guest_string, sizeof(guest_string)) ||
                !append_snprintf(output, output_capacity, &used, spec,
                                 guest_string))
                return 0;
        } else if (conversion == 'p') {
            if (length != LOG_LEN_NONE ||
                !get_word(frame, word_index++, &word) ||
                !append_snprintf(output, output_capacity, &used, spec,
                                 (void *)(uintptr_t)word))
                return 0;
        } else if (conversion == 'c') {
            if (length != LOG_LEN_NONE ||
                !get_word(frame, word_index++, &word) ||
                !append_snprintf(output, output_capacity, &used, spec,
                                 (int32_t)word))
                return 0;
        } else if (strchr("fFeEgGaA", conversion)) {
            double value;
            if (length != LOG_LEN_NONE && length != LOG_LEN_L) return 0;
            if (!get_u64_even(frame, &word_index, &wide)) return 0;
            memcpy(&value, &wide, sizeof(value));
            if (!append_snprintf(output, output_capacity, &used, spec, value))
                return 0;
        } else if (strchr("diuoxX", conversion)) {
            int is_signed = (conversion == 'd' || conversion == 'i');
            if (length == LOG_LEN_LL) {
                if (!get_u64_even(frame, &word_index, &wide)) return 0;
                if (is_signed) {
                    long long value = (long long)(int64_t)wide;
                    if (!append_snprintf(output, output_capacity, &used, spec,
                                         value)) return 0;
                } else {
                    unsigned long long value = (unsigned long long)wide;
                    if (!append_snprintf(output, output_capacity, &used, spec,
                                         value)) return 0;
                }
            } else {
                if (!get_word(frame, word_index++, &word)) return 0;
                if (is_signed) {
                    int32_t value = (int32_t)word;
                    switch (length) {
                        case LOG_LEN_L: {
                            long v = (long)value;
                            if (!append_snprintf(output, output_capacity,
                                                 &used, spec, v)) return 0;
                            break;
                        }
                        default:
                            if (!append_snprintf(output, output_capacity,
                                                 &used, spec, (int)value))
                                return 0;
                    }
                } else {
                    switch (length) {
                        case LOG_LEN_L: {
                            unsigned long v = (unsigned long)word;
                            if (!append_snprintf(output, output_capacity,
                                                 &used, spec, v)) return 0;
                            break;
                        }
                        case LOG_LEN_J: return 0;
                        default:
                            if (!append_snprintf(output, output_capacity,
                                                 &used, spec,
                                                 (unsigned int)word))
                                return 0;
                    }
                }
            }
        }
    }
    if (output_length) *output_length = used;
    return 1;
}
