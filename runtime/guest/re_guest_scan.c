#include "re_guest_scan.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    SCAN_LEN_NONE, SCAN_LEN_HH, SCAN_LEN_H, SCAN_LEN_L,
    SCAN_LEN_LL, SCAN_LEN_Z, SCAN_LEN_T, SCAN_LEN_J
} ScanLength;

static int scan_space(unsigned char c) { return c == ' ' || c == '\t' ||
    c == '\n' || c == '\r' || c == '\v' || c == '\f'; }

static int get_arg(const S3EAAPCS32Frame *frame, uint32_t *index,
                   uint32_t *value) {
    if (!frame || !index || !value || *index == UINT32_MAX) return 0;
    if (s3e_aapcs32_arg_word(frame, *index, value) != S3E_AAPCS32_OK)
        return 0;
    ++*index;
    return 1;
}

static int guest_write(REGUESTMEMBUS *bus, uint32_t address,
                       const void *bytes, size_t length) {
    void *host = NULL;
    if (!bus || !bus->memory || !address || (!bytes && length) ||
        re_guest_memory_resolve(bus->memory, address, length,
                                RE_GUEST_MEM_WRITE, &host) != RE_GUEST_MEM_OK)
        return 0;
    if (length) memcpy(host, bytes, length);
    return 1;
}

static int store_integer(REGUESTMEMBUS *bus, uint32_t address,
                         uint64_t value, ScanLength length,
                         int is_signed) {
    uint8_t out[8];
    size_t n = 4u;
    if (length == SCAN_LEN_HH) n = 1u;
    else if (length == SCAN_LEN_H) n = 2u;
    else if (length == SCAN_LEN_LL || length == SCAN_LEN_J) n = 8u;
    for (size_t i = 0; i < n; ++i) out[i] = (uint8_t)(value >> (i * 8u));
    (void)is_signed;
    return guest_write(bus, address, out, n);
}

static int scanset_build(const char **format, uint8_t set[256]) {
    const unsigned char *p = (const unsigned char *)*format;
    int invert = 0, any = 0;
    memset(set, 0, 256u);
    if (*p == '^') { invert = 1; ++p; }
    if (*p == ']') { set[*p++] = 1u; any = 1; }
    while (*p && *p != ']') {
        unsigned char first = *p++;
        if (*p == '-' && p[1] && p[1] != ']') {
            unsigned char last;
            ++p; last = *p++;
            if (last < first) return 0;
            for (unsigned v = first; v <= last; ++v) set[v] = 1u;
        } else set[first] = 1u;
        any = 1;
    }
    if (*p != ']' || !any) return 0;
    *format = (const char *)(p + 1);
    if (invert) for (size_t i = 0; i < 256u; ++i) set[i] = (uint8_t)!set[i];
    return 1;
}

static int parse_integer(const char *input, size_t limit, int base,
                         int is_signed, uint64_t *value, size_t *used) {
    char token[RE_GUEST_SCAN_TOKEN_CAP];
    char *end = NULL;
    size_t n = 0u;
    while (n < limit && n + 1u < sizeof(token)) {
        unsigned char ch = (unsigned char)input[n];
        if (!ch || scan_space(ch)) break;
        if (!(ch == '+' || ch == '-' || ch == 'x' || ch == 'X' ||
              (ch >= '0' && ch <= '9') ||
              (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')))
            break;
        token[n] = (char)ch;
        ++n;
    }
    if (!n) return 0;
    token[n] = '\0';
    errno = 0;
    if (is_signed) {
        long long v = strtoll(token, &end, base);
        if (end == token || errno == ERANGE) return 0;
        *value = (uint64_t)(int64_t)v;
    } else {
        unsigned long long v = strtoull(token, &end, base);
        if (end == token || errno == ERANGE) return 0;
        *value = (uint64_t)v;
    }
    *used = (size_t)(end - token);
    return *used != 0u;
}

int re_guest_scan_guest_sscanf(const S3EAAPCS32Frame *frame,
                               REGUESTMEMBUS *bus,
                               uint32_t first_word_index,
                               const char *input, const char *format,
                               uint32_t *assignments,
                               size_t *input_consumed,
                               int *input_failure) {
    const char *in;
    const char *fp;
    uint32_t arg_index = first_word_index, assigned = 0u;
    int failed_input = 0;
    if (assignments) *assignments = 0u;
    if (input_consumed) *input_consumed = 0u;
    if (input_failure) *input_failure = 0;
    if (!frame || !bus || !bus->memory || !input || !format) return 0;
    in = input; fp = format;

    while (*fp) {
        if (scan_space((unsigned char)*fp)) {
            while (scan_space((unsigned char)*fp)) ++fp;
            while (scan_space((unsigned char)*in)) ++in;
            continue;
        }
        if (*fp != '%') {
            if (*in != *fp) { failed_input = (*in == '\0'); break; }
            ++in; ++fp; continue;
        }
        ++fp;
        if (*fp == '%') {
            if (*in != '%') { failed_input = (*in == '\0'); break; }
            ++in; ++fp; continue;
        }
        if (!*fp) return 0;

        int suppress = 0;
        size_t width = SIZE_MAX;
        ScanLength length = SCAN_LEN_NONE;
        if (*fp == '*') { suppress = 1; ++fp; }
        if (*fp >= '0' && *fp <= '9') {
            width = 0u;
            while (*fp >= '0' && *fp <= '9') {
                unsigned digit = (unsigned)(*fp++ - '0');
                if (width > 4096u) return 0;
                width = width * 10u + digit;
            }
            if (!width || width > 4096u) return 0;
        }
        if (*fp == 'h') {
            ++fp; length = SCAN_LEN_H;
            if (*fp == 'h') { ++fp; length = SCAN_LEN_HH; }
        } else if (*fp == 'l') {
            ++fp; length = SCAN_LEN_L;
            if (*fp == 'l') { ++fp; length = SCAN_LEN_LL; }
        } else if (*fp == 'z') { ++fp; length = SCAN_LEN_Z; }
        else if (*fp == 't') { ++fp; length = SCAN_LEN_T; }
        else if (*fp == 'j') { ++fp; length = SCAN_LEN_J; }
        if (!*fp) return 0;

        char conv = *fp++;
        if (conv != 'c' && conv != '[' && conv != 'n')
            while (scan_space((unsigned char)*in)) ++in;
        if (conv == 'n') {
            uint32_t dst;
            if (!suppress) {
                if (!get_arg(frame, &arg_index, &dst) ||
                    !store_integer(bus, dst, (uint64_t)(in - input), length, 1))
                    return 0;
            }
            continue;
        }
        if (conv == 'c' || conv == 's' || conv == '[') {
            uint8_t set[256];
            size_t n = 0u, max = width;
            char temp[RE_GUEST_SCAN_TOKEN_CAP];
            if (conv == '[') {
                if (!scanset_build(&fp, set)) return 0;
            }
            if (conv == 'c' && width == SIZE_MAX) max = 1u;
            if ((conv == 's' || conv == '[') && max == SIZE_MAX)
                max = sizeof(temp) - 1u;
            if (conv == 'c' && length != SCAN_LEN_NONE) return 0;
            if (conv == 's' && length != SCAN_LEN_NONE && length != SCAN_LEN_L)
                return 0;
            while (n < max && n + 1u < sizeof(temp) && in[n]) {
                unsigned char ch = (unsigned char)in[n];
                if (conv == 's' && scan_space(ch)) break;
                if (conv == '[' && !set[ch]) break;
                temp[n] = (char)ch; ++n;
                if (conv == 'c' && n == max) break;
            }
            if (!n || (conv == 'c' && n != max)) {
                failed_input = (*in == '\0'); break;
            }
            if (!suppress) {
                uint32_t dst;
                if (!get_arg(frame, &arg_index, &dst)) return 0;
                if (conv != 'c') temp[n++] = '\0';
                if (!guest_write(bus, dst, temp, n)) return 0;
                ++assigned;
            }
            in += conv == 'c' ? n : (n - (suppress ? 0u : 1u));
            continue;
        }
        if (strchr("diuoxXp", conv)) {
            uint64_t value;
            size_t used = 0u;
            int base = conv == 'o' ? 8 : ((conv == 'x' || conv == 'X' || conv == 'p') ? 16 :
                       (conv == 'i' ? 0 : 10));
            int sign = conv == 'd' || conv == 'i';
            size_t max = width == SIZE_MAX ? RE_GUEST_SCAN_TOKEN_CAP - 1u : width;
            if (length == SCAN_LEN_L || length == SCAN_LEN_Z ||
                length == SCAN_LEN_T || length == SCAN_LEN_J) {
                /* ARM32 long/size_t/ptrdiff_t remain 32-bit; intmax_t is 64-bit. */
                if (length == SCAN_LEN_J) { /* retained as a 64-bit guest integer */ }
            }
            if (!parse_integer(in, max, base, sign, &value, &used)) {
                failed_input = (*in == '\0'); break;
            }
            if (!suppress) {
                uint32_t dst;
                if (!get_arg(frame, &arg_index, &dst) ||
                    !store_integer(bus, dst, value, conv == 'p' ? SCAN_LEN_NONE : length, sign))
                    return 0;
                ++assigned;
            }
            in += used;
            continue;
        }
        if (strchr("fFeEgGaA", conv)) {
            char token[RE_GUEST_SCAN_TOKEN_CAP], *end = NULL;
            size_t n = 0u, max = width == SIZE_MAX ? sizeof(token) - 1u : width;
            double value;
            while (n < max && n + 1u < sizeof(token) && in[n] &&
                   !scan_space((unsigned char)in[n])) {
                unsigned char ch = (unsigned char)in[n];
                if (!(ch == '+' || ch == '-' || ch == '.' || ch == 'e' ||
                      ch == 'E' || ch == 'p' || ch == 'P' || ch == 'x' ||
                      ch == 'X' || ch == 'i' || ch == 'I' || ch == 'n' ||
                      ch == 'N' || ch == 'f' || ch == 'F' || ch == 'a' ||
                      ch == 'A' || (ch >= '0' && ch <= '9'))) break;
                token[n] = (char)ch; ++n;
            }
            if (!n) { failed_input = (*in == '\0'); break; }
            token[n] = '\0'; errno = 0; value = strtod(token, &end);
            if (end == token || errno == ERANGE) { failed_input = 0; break; }
            if (!suppress) {
                uint32_t dst;
                if (!get_arg(frame, &arg_index, &dst)) return 0;
                if (length == SCAN_LEN_L || length == SCAN_LEN_LL) {
                    uint64_t bits; memcpy(&bits, &value, sizeof(bits));
                    if (!guest_write(bus, dst, &bits, sizeof(bits))) return 0;
                } else {
                    float f = (float)value;
                    if (!guest_write(bus, dst, &f, sizeof(f))) return 0;
                }
                ++assigned;
            }
            in += (size_t)(end - token);
            continue;
        }
        return 0; /* Unsupported conversion (including positional extensions). */
    }
    if (assignments) *assignments = assigned;
    if (input_consumed) *input_consumed = (size_t)(in - input);
    if (input_failure) *input_failure = failed_input;
    return 1;
}
