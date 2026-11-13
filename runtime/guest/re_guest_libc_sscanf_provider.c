#include "re_guest_libc_sscanf_provider.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "re_guest_memory_bus.h"
#include "s3e_aapcs32.h"

typedef enum {
    SCAN_LEN_NONE, SCAN_LEN_HH, SCAN_LEN_H, SCAN_LEN_L,
    SCAN_LEN_LL, SCAN_LEN_Z, SCAN_LEN_T, SCAN_LEN_J, SCAN_LEN_CAP_L
} ScanLength;

static int read_guest_cstring(const REGUESTMEMBUS *bus, uint32_t address,
                              char *out, size_t capacity, size_t *length) {
    if (!bus || !address || !out || capacity == 0u) return 0;
    for (size_t i = 0; i < capacity; ++i) {
        uint64_t at = (uint64_t)address + i;
        uint8_t byte;
        if (at > UINT32_MAX ||
            re_guest_memory_read8(bus, (uint32_t)at, &byte) != RE_GUEST_MEM_OK)
            return 0;
        out[i] = (char)byte;
        if (!byte) {
            if (length) *length = i;
            return 1;
        }
    }
    return 0;
}

static int fetch_argument_word(const S3EAAPCS32Frame *frame,
                               uint32_t *word_index, uint32_t *value) {
    if (!frame || !word_index || !value ||
        s3e_aapcs32_arg_word(frame, (*word_index)++, value) != S3E_AAPCS32_OK)
        return 0;
    return 1;
}

static int store_guest(const REGUESTMEMBUS *bus, uint32_t address,
                       const void *value, size_t size) {
    void *host = NULL;
    if (!address || !value || size == 0u ||
        re_guest_memory_resolve(bus->memory, address, size,
            RE_GUEST_MEM_WRITE, &host) != RE_GUEST_MEM_OK)
        return 0;
    memcpy(host, value, size);
    return 1;
}

static int skip_input_space(const char *input, size_t input_length,
                            size_t *position) {
    while (*position < input_length &&
           isspace((unsigned char)input[*position]))
        ++*position;
    return *position < input_length;
}

static int parse_width(const char **format, uint32_t *width, int *specified) {
    uint32_t value = 0u;
    *specified = 0;
    while (**format >= '0' && **format <= '9') {
        *specified = 1;
        value = value * 10u + (uint32_t)(**format - '0');
        if (value > RE_GUEST_LIBC_SSCANF_MAX_BYTES) return 0;
        ++*format;
    }
    *width = value;
    return 1;
}

static int store_signed(const REGUESTMEMBUS *bus, uint32_t address,
                        int64_t value, ScanLength length) {
    switch (length) {
        case SCAN_LEN_HH: { int8_t v = (int8_t)value; return store_guest(bus,address,&v,sizeof(v)); }
        case SCAN_LEN_H: { int16_t v = (int16_t)value; return store_guest(bus,address,&v,sizeof(v)); }
        case SCAN_LEN_LL: case SCAN_LEN_J: return store_guest(bus,address,&value,sizeof(value));
        case SCAN_LEN_CAP_L: return 0;
        default: { int32_t v = (int32_t)value; return store_guest(bus,address,&v,sizeof(v)); }
    }
}

static int store_unsigned(const REGUESTMEMBUS *bus, uint32_t address,
                          uint64_t value, ScanLength length) {
    switch (length) {
        case SCAN_LEN_HH: { uint8_t v = (uint8_t)value; return store_guest(bus,address,&v,sizeof(v)); }
        case SCAN_LEN_H: { uint16_t v = (uint16_t)value; return store_guest(bus,address,&v,sizeof(v)); }
        case SCAN_LEN_LL: case SCAN_LEN_J: return store_guest(bus,address,&value,sizeof(value));
        case SCAN_LEN_CAP_L: return 0;
        default: { uint32_t v = (uint32_t)value; return store_guest(bus,address,&v,sizeof(v)); }
    }
}

static REGUESTIMPORTStatus sscanf_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLIBCSSCANFProvider *provider = (REGUESTLIBCSSCANFProvider *)user;
    char input[RE_GUEST_LIBC_SSCANF_MAX_BYTES];
    char format[RE_GUEST_LIBC_SSCANF_MAX_BYTES];
    size_t input_length = 0u, format_length = 0u, in_pos = 0u;
    uint32_t arg_word_index = 2u;
    int assignments = 0, input_failure = 0;
    const char *f;

    if (!frame || !bus || !bus->memory || !provider ||
        !provider->initialized || !result)
        return RE_IMPORT_E_HANDLER;
    if (!read_guest_cstring(bus, frame->r[0], input, sizeof(input), &input_length) ||
        !read_guest_cstring(bus, frame->r[1], format, sizeof(format), &format_length))
        return RE_IMPORT_E_HANDLER;
    (void)format_length;
    f = format;

    while (*f) {
        int suppress = 0, width_specified = 0;
        uint32_t width = RE_GUEST_LIBC_SSCANF_MAX_BYTES;
        ScanLength length = SCAN_LEN_NONE;
        char conversion;
        uint32_t destination = 0u;
        char token[RE_GUEST_LIBC_SSCANF_MAX_BYTES + 1u];
        char *end = NULL;
        size_t token_length = 0u;

        if (isspace((unsigned char)*f)) {
            while (isspace((unsigned char)*f)) ++f;
            (void)skip_input_space(input, input_length, &in_pos);
            continue;
        }
        if (*f != '%') {
            if (in_pos >= input_length) { input_failure = 1; break; }
            if (input[in_pos] != *f) break;
            ++in_pos; ++f;
            continue;
        }
        ++f;
        if (*f == '%') {
            if (in_pos >= input_length) { input_failure = 1; break; }
            if (input[in_pos] != '%') break;
            ++in_pos; ++f;
            continue;
        }
        if (*f == '*') { suppress = 1; ++f; }
        if (!parse_width(&f, &width, &width_specified))
            return RE_IMPORT_E_HANDLER;
        if (!width_specified) width = RE_GUEST_LIBC_SSCANF_MAX_BYTES;
        if (*f == 'h') {
            ++f; length = SCAN_LEN_H;
            if (*f == 'h') { ++f; length = SCAN_LEN_HH; }
        } else if (*f == 'l') {
            ++f; length = SCAN_LEN_L;
            if (*f == 'l') { ++f; length = SCAN_LEN_LL; }
        } else if (*f == 'z') { ++f; length = SCAN_LEN_Z; }
        else if (*f == 't') { ++f; length = SCAN_LEN_T; }
        else if (*f == 'j') { ++f; length = SCAN_LEN_J; }
        else if (*f == 'L') { ++f; length = SCAN_LEN_CAP_L; }
        conversion = *f++;
        if (!conversion || width > RE_GUEST_LIBC_SSCANF_MAX_BYTES)
            return RE_IMPORT_E_HANDLER;

        if (conversion != 'c' && conversion != 'n')
            (void)skip_input_space(input, input_length, &in_pos);

        if (conversion == 'n') {
            if (!suppress) {
                if (!fetch_argument_word(frame, &arg_word_index, &destination) ||
                    !store_signed(bus, destination, (int64_t)in_pos, length))
                    return RE_IMPORT_E_HANDLER;
            }
            continue;
        }

        if (conversion == 's' || conversion == 'c') {
            size_t copy_length;
            if (length != SCAN_LEN_NONE) return RE_IMPORT_E_HANDLER;
            if (conversion == 'c' && !width_specified) width = 1u;
            if (in_pos >= input_length) { input_failure = 1; break; }
            if (conversion == 's') {
                while (in_pos + token_length < input_length &&
                       token_length < width &&
                       !isspace((unsigned char)input[in_pos + token_length]))
                    ++token_length;
                if (!token_length) break;
                copy_length = token_length + 1u;
            } else {
                copy_length = width;
                if (copy_length > input_length - in_pos) {
                    input_failure = 1;
                    break;
                }
                token_length = copy_length;
            }
            if (!suppress) {
                if (!fetch_argument_word(frame, &arg_word_index, &destination))
                    return RE_IMPORT_E_HANDLER;
                if (conversion == 's') {
                    memcpy(token, input + in_pos, token_length);
                    token[token_length] = '\0';
                    if (!store_guest(bus, destination, token, copy_length))
                        return RE_IMPORT_E_HANDLER;
                } else if (!store_guest(bus, destination, input + in_pos,
                                        copy_length)) {
                    return RE_IMPORT_E_HANDLER;
                }
                ++assignments;
            }
            in_pos += token_length;
            continue;
        }

        if (conversion == 'd' || conversion == 'i' || conversion == 'u' ||
            conversion == 'o' || conversion == 'x' || conversion == 'X' ||
            conversion == 'p') {
            int base = conversion == 'i' ? 0 :
                       conversion == 'o' ? 8 :
                       (conversion == 'x' || conversion == 'X' || conversion == 'p') ? 16 : 10;
            size_t available = input_length - in_pos;
            if (width < available) available = width;
            if (!available) { input_failure = 1; break; }
            memcpy(token, input + in_pos, available);
            token[available] = '\0';
            if (conversion == 'd' || conversion == 'i') {
                long long value = strtoll(token, &end, base);
                token_length = (size_t)(end - token);
                if (!token_length) break;
                if (!suppress) {
                    if (!fetch_argument_word(frame, &arg_word_index, &destination) ||
                        !store_signed(bus, destination, (int64_t)value, length))
                        return RE_IMPORT_E_HANDLER;
                    ++assignments;
                }
            } else {
                unsigned long long value = strtoull(token, &end, base);
                token_length = (size_t)(end - token);
                if (!token_length) break;
                if (!suppress) {
                    if (!fetch_argument_word(frame, &arg_word_index, &destination) ||
                        !store_unsigned(bus, destination, (uint64_t)value, length))
                        return RE_IMPORT_E_HANDLER;
                    ++assignments;
                }
            }
            in_pos += token_length;
            continue;
        }

        if (conversion == 'f' || conversion == 'F' || conversion == 'e' ||
            conversion == 'E' || conversion == 'g' || conversion == 'G' ||
            conversion == 'a' || conversion == 'A') {
            double value;
            size_t available = input_length - in_pos;
            if (length == SCAN_LEN_HH || length == SCAN_LEN_H ||
                length == SCAN_LEN_LL || length == SCAN_LEN_Z ||
                length == SCAN_LEN_T || length == SCAN_LEN_J ||
                length == SCAN_LEN_CAP_L)
                return RE_IMPORT_E_HANDLER;
            if (width < available) available = width;
            if (!available) { input_failure = 1; break; }
            memcpy(token, input + in_pos, available);
            token[available] = '\0';
            errno = 0;
            value = strtod(token, &end);
            token_length = (size_t)(end - token);
            if (!token_length) break;
            if (!suppress) {
                if (!fetch_argument_word(frame, &arg_word_index, &destination))
                    return RE_IMPORT_E_HANDLER;
                if (length == SCAN_LEN_L) {
                    if (!store_guest(bus, destination, &value, sizeof(value)))
                        return RE_IMPORT_E_HANDLER;
                } else {
                    float fvalue = (float)value;
                    if (!store_guest(bus, destination, &fvalue, sizeof(fvalue)))
                        return RE_IMPORT_E_HANDLER;
                }
                ++assignments;
            }
            in_pos += token_length;
            continue;
        }

        return RE_IMPORT_E_HANDLER; /* scansets and unknown conversions are deferred. */
    }

    result->r0 = (uint32_t)((assignments == 0 && input_failure) ? -1 : assignments);
    result->r1 = result->r2 = result->r3 = 0u;
    return RE_IMPORT_OK;
}

static int find_undefined_symbol(
    const REGUESTELF32LINKSYMBOL *symbols, size_t count,
    const char *name, size_t *found) {
    size_t match = count;
    if (!found) return 0;
    *found = count;
    for (size_t i = 0; i < count; ++i) {
        const REGUESTELF32LINKSYMBOL *symbol = &symbols[i];
        if (!memchr(symbol->name, '\0', sizeof(symbol->name))) return 0;
        if (strcmp(symbol->name, name) != 0 || symbol->section_index != 0u ||
            (symbol->binding != RE_GUEST_STB_GLOBAL &&
             symbol->binding != RE_GUEST_STB_WEAK) ||
            (symbol->visibility != RE_GUEST_STV_DEFAULT &&
             symbol->visibility != RE_GUEST_STV_PROTECTED) ||
            (symbol->type != 0u && symbol->type != 2u))
            continue;
        if (match != count) return 0;
        match = i;
    }
    if (match == count) return 0;
    *found = match;
    return 1;
}

REGUESTLIBCSSCANFStatus re_guest_libc_sscanf_provider_init(
    REGUESTLIBCSSCANFProvider *provider) {
    if (!provider) return RE_GUEST_LIBC_SSCANF_E_ARGUMENT;
    provider->initialized = 1u;
    return RE_GUEST_LIBC_SSCANF_OK;
}

REGUESTLIBCSSCANFStatus re_guest_libc_sscanf_provider_bind_elf_symbols(
    REGUESTLIBCSSCANFProvider *provider, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index) {
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[1];
    size_t index = symbol_count;
    uint32_t pointer = 0u;
    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !provider->initialized || !imports || !symbols ||
        !exports || !export_count || export_capacity < 1u)
        return RE_GUEST_LIBC_SSCANF_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count >= RE_IMPORT_MAX_BINDINGS)
        return RE_GUEST_LIBC_SSCANF_E_CAPACITY;
    if (!find_undefined_symbol(symbols, symbol_count, "sscanf", &index)) {
        if (failed_symbol_index) *failed_symbol_index = index;
        return RE_GUEST_LIBC_SSCANF_E_SYMBOL;
    }
    backup = *imports;
    if (re_guest_import_bind_mapped(imports, (uint32_t)index,
            sscanf_handler, provider, &pointer) != RE_IMPORT_OK) {
        *imports = backup;
        return RE_GUEST_LIBC_SSCANF_E_IMPORT;
    }
    pending[0] = (REGUESTSYMBOL){
        "sscanf", pointer, RE_GUEST_STB_GLOBAL, RE_GUEST_STV_DEFAULT,
        1u, NULL, "libc.so"
    };
    memcpy(exports, pending, sizeof(pending));
    *export_count = 1u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_LIBC_SSCANF_OK;
}
