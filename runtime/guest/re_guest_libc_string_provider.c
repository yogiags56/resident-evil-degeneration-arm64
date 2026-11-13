#include "re_guest_libc_string_provider.h"

#include <limits.h>
#include <string.h>

typedef enum {
    LIBC_MEMCPY, LIBC_MEMMOVE, LIBC_MEMSET, LIBC_STRLEN,
    LIBC_STRCMP, LIBC_STRNCMP, LIBC_STRCPY, LIBC_STRNCPY,
    LIBC_STRNCAT, LIBC_STRCHR, LIBC_STRSTR, LIBC_STRCASECMP,
    LIBC_STRNCASECMP, LIBC_ATOI, LIBC_STRTOK
} LIBCStringKind;

typedef struct { LIBCStringKind kind; } LIBCStringContext;

static const char *const k_names[RE_GUEST_LIBC_STRING_SYMBOL_COUNT] = {
    "memcpy", "memmove", "memset", "strlen", "strcmp", "strncmp",
    "strcpy", "strncpy", "strncat", "strchr", "strstr",
    "strcasecmp", "strncasecmp", "atoi", "strtok"
};

static const LIBCStringContext k_contexts[RE_GUEST_LIBC_STRING_SYMBOL_COUNT] = {
    {LIBC_MEMCPY}, {LIBC_MEMMOVE}, {LIBC_MEMSET}, {LIBC_STRLEN},
    {LIBC_STRCMP}, {LIBC_STRNCMP}, {LIBC_STRCPY}, {LIBC_STRNCPY},
    {LIBC_STRNCAT}, {LIBC_STRCHR}, {LIBC_STRSTR}, {LIBC_STRCASECMP},
    {LIBC_STRNCASECMP}, {LIBC_ATOI}, {LIBC_STRTOK}
};

static int add_address(uint32_t base, uint32_t offset, uint32_t *out) {
    uint64_t value = (uint64_t)base + offset;
    if (!out || value > UINT32_MAX) return 0;
    *out = (uint32_t)value;
    return 1;
}

static int read_byte(const REGUESTMEMBUS *bus, uint32_t base,
                     uint32_t offset, uint8_t *value) {
    uint32_t address;
    return add_address(base, offset, &address) &&
           re_guest_memory_read8(bus, address, value) == RE_GUEST_MEM_OK;
}

static int write_byte(const REGUESTMEMBUS *bus, uint32_t base,
                      uint32_t offset, uint8_t value) {
    uint32_t address;
    return add_address(base, offset, &address) &&
           re_guest_memory_write8(bus, address, value) == RE_GUEST_MEM_OK;
}

static int cstring_length(const REGUESTMEMBUS *bus, uint32_t address,
                          uint32_t *length) {
    uint8_t c;
    if (!bus || !length || !address) return 0;
    for (uint32_t i = 0; i < RE_GUEST_LIBC_STRING_MAX_BYTES; ++i) {
        if (!read_byte(bus, address, i, &c)) return 0;
        if (!c) {
            *length = i;
            return 1;
        }
    }
    return 0;
}

static uint8_t ascii_lower(uint8_t c) {
    return (c >= (uint8_t)'A' && c <= (uint8_t)'Z')
        ? (uint8_t)(c + ((uint8_t)'a' - (uint8_t)'A')) : c;
}

static int is_ascii_space(uint8_t c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
           c == '\f' || c == '\v';
}

/* Returns 1 for a delimiter, 0 for a non-delimiter, and -1 on bad memory. */
static int is_delimiter(const REGUESTMEMBUS *bus, uint32_t delim,
                        uint8_t value) {
    uint8_t c;
    if (!delim) return -1;
    for (uint32_t i = 0; i < RE_GUEST_LIBC_STRING_MAX_BYTES; ++i) {
        if (!read_byte(bus, delim, i, &c)) return -1;
        if (!c) return 0;
        if (c == value) return 1;
    }
    return -1;
}

static REGUESTIMPORTStatus compare_strings(
    const REGUESTMEMBUS *bus, uint32_t left, uint32_t right,
    uint32_t limit, int insensitive, uint32_t *result) {
    uint32_t max = limit ? limit : RE_GUEST_LIBC_STRING_MAX_BYTES;
    uint8_t a, b;
    if (!left || !right || !result ||
        (limit > RE_GUEST_LIBC_STRING_MAX_BYTES))
        return RE_IMPORT_E_HANDLER;
    for (uint32_t i = 0; i < max; ++i) {
        if (!read_byte(bus, left, i, &a) || !read_byte(bus, right, i, &b))
            return RE_IMPORT_E_HANDLER;
        if (insensitive) {
            a = ascii_lower(a);
            b = ascii_lower(b);
        }
        if (a != b) {
            *result = (uint32_t)((int32_t)a - (int32_t)b);
            return RE_IMPORT_OK;
        }
        if (!a) {
            *result = 0u;
            return RE_IMPORT_OK;
        }
    }
    if (limit) {
        *result = 0u;
        return RE_IMPORT_OK;
    }
    return RE_IMPORT_E_HANDLER;
}

static REGUESTIMPORTStatus libc_string_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLIBCStringProvider *provider = NULL;
    const REGUESTLIBCStringContext *binding =
        (const REGUESTLIBCStringContext *)user;
    LIBCStringContext local_context;
    const LIBCStringContext *context = &local_context;
    LIBCStringKind kind;
    uint32_t dst, src, count, length, other_length, out;
    uint8_t a, b;
    void *host_dst = NULL, *host_src = NULL;

    if (!frame || !bus || !bus->memory || !binding || !result)
        return RE_IMPORT_E_HANDLER;
    kind = (LIBCStringKind)binding->kind;
    local_context.kind = kind;
    provider = binding->provider;
    result->r0 = result->r1 = result->r2 = result->r3 = 0u;
    dst = frame->r[0];
    src = frame->r[1];
    count = frame->r[2];

    switch (kind) {
        case LIBC_MEMCPY:
        case LIBC_MEMMOVE:
            if (count) {
                if (re_guest_memory_resolve(bus->memory, dst, count,
                        RE_GUEST_MEM_WRITE, &host_dst) != RE_GUEST_MEM_OK ||
                    re_guest_memory_resolve(bus->memory, src, count,
                        RE_GUEST_MEM_READ, &host_src) != RE_GUEST_MEM_OK)
                    return RE_IMPORT_E_HANDLER;
                if (context->kind == LIBC_MEMMOVE)
                    memmove(host_dst, host_src, count);
                else
                    memcpy(host_dst, host_src, count);
            }
            result->r0 = dst;
            break;
        case LIBC_MEMSET:
            if (count) {
                if (re_guest_memory_resolve(bus->memory, dst, count,
                        RE_GUEST_MEM_WRITE, &host_dst) != RE_GUEST_MEM_OK)
                    return RE_IMPORT_E_HANDLER;
                memset(host_dst, (int)(frame->r[1] & 0xffu), count);
            }
            result->r0 = dst;
            break;
        case LIBC_STRLEN:
            if (!cstring_length(bus, dst, &length)) return RE_IMPORT_E_HANDLER;
            result->r0 = length;
            break;
        case LIBC_STRCMP:
        case LIBC_STRNCMP:
        case LIBC_STRCASECMP:
        case LIBC_STRNCASECMP:
            if ((kind == LIBC_STRNCMP || kind == LIBC_STRNCASECMP) &&
                count == 0u) {
                result->r0 = 0u;
                break;
            }
            if (compare_strings(bus, dst, src,
                    (context->kind == LIBC_STRNCMP ||
                     context->kind == LIBC_STRNCASECMP) ? count : 0u,
                    context->kind == LIBC_STRCASECMP ||
                    context->kind == LIBC_STRNCASECMP, &out) != RE_IMPORT_OK)
                return RE_IMPORT_E_HANDLER;
            result->r0 = out;
            break;
        case LIBC_STRCPY:
            if (!dst || !src) return RE_IMPORT_E_HANDLER;
            for (uint32_t i = 0; i < RE_GUEST_LIBC_STRING_MAX_BYTES; ++i) {
                if (!read_byte(bus, src, i, &a) || !write_byte(bus, dst, i, a))
                    return RE_IMPORT_E_HANDLER;
                if (!a) {
                    result->r0 = dst;
                    return RE_IMPORT_OK;
                }
            }
            return RE_IMPORT_E_HANDLER;
        case LIBC_STRNCPY:
            if (!dst || (!src && count)) return RE_IMPORT_E_HANDLER;
            if (count > RE_GUEST_LIBC_STRING_MAX_BYTES) return RE_IMPORT_E_HANDLER;
            for (uint32_t i = 0; i < count; ++i) {
                if (src && read_byte(bus, src, i, &a)) {
                    if (!a) src = 0u;
                } else if (src) {
                    return RE_IMPORT_E_HANDLER;
                } else {
                    a = 0u;
                }
                if (!write_byte(bus, dst, i, a)) return RE_IMPORT_E_HANDLER;
            }
            result->r0 = dst;
            break;
        case LIBC_STRNCAT:
            if (!dst || !src || count > RE_GUEST_LIBC_STRING_MAX_BYTES ||
                !cstring_length(bus, dst, &length))
                return RE_IMPORT_E_HANDLER;
            for (uint32_t i = 0; i < count; ++i) {
                if (!read_byte(bus, src, i, &a)) return RE_IMPORT_E_HANDLER;
                if (!a) break;
                if (!write_byte(bus, dst, length++, a)) return RE_IMPORT_E_HANDLER;
            }
            if (!write_byte(bus, dst, length, 0u)) return RE_IMPORT_E_HANDLER;
            result->r0 = dst;
            break;
        case LIBC_STRCHR:
            if (!dst) return RE_IMPORT_E_HANDLER;
            for (uint32_t i = 0; i < RE_GUEST_LIBC_STRING_MAX_BYTES; ++i) {
                if (!read_byte(bus, dst, i, &a)) return RE_IMPORT_E_HANDLER;
                if (a == (uint8_t)src) {
                    if (!add_address(dst, i, &result->r0))
                        return RE_IMPORT_E_HANDLER;
                    return RE_IMPORT_OK;
                }
                if (!a) break;
            }
            result->r0 = 0u;
            break;
        case LIBC_STRSTR:
            if (!dst || !src || !cstring_length(bus, src, &other_length) ||
                !cstring_length(bus, dst, &length))
                return RE_IMPORT_E_HANDLER;
            if (!other_length) {
                result->r0 = dst;
                break;
            }
            if (other_length <= length) {
                for (uint32_t i = 0; i <= length - other_length; ++i) {
                    uint32_t j;
                    for (j = 0; j < other_length; ++j) {
                        if (!read_byte(bus, dst, i + j, &a) ||
                            !read_byte(bus, src, j, &b))
                            return RE_IMPORT_E_HANDLER;
                        if (a != b) break;
                    }
                    if (j == other_length) {
                        if (!add_address(dst, i, &result->r0))
                            return RE_IMPORT_E_HANDLER;
                        return RE_IMPORT_OK;
                    }
                }
            }
            result->r0 = 0u;
            break;
        case LIBC_ATOI: {
            uint32_t i = 0u;
            int negative = 0, have_digit = 0;
            uint64_t value = 0u;
            if (!dst || !cstring_length(bus, dst, &length))
                return RE_IMPORT_E_HANDLER;
            if (!length) {
                result->r0 = 0u;
                break;
            }
            for (; i < length; ++i) {
                if (!read_byte(bus, dst, i, &a)) return RE_IMPORT_E_HANDLER;
                if (!is_ascii_space(a)) break;
            }
            if (a == '+' || a == '-') {
                negative = (a == '-');
                ++i;
            }
            for (; i < length; ++i) {
                if (!read_byte(bus, dst, i, &a)) return RE_IMPORT_E_HANDLER;
                if (a < '0' || a > '9') break;
                have_digit = 1;
                value = value * 10u + (uint64_t)(a - '0');
                if (value > (uint64_t)INT32_MAX + (negative ? 1u : 0u))
                    value = (uint64_t)INT32_MAX + (negative ? 1u : 0u);
            }
            if (!have_digit) result->r0 = 0u;
            else if (negative && value == (uint64_t)INT32_MAX + 1u)
                result->r0 = (uint32_t)INT32_MIN;
            else result->r0 = negative ? (uint32_t)-(int32_t)value : (uint32_t)value;
            break;
        }
        case LIBC_STRTOK: {
            uint32_t cursor = dst ? dst : (provider ? provider->strtok_next : 0u);
            uint32_t start;
            if (!provider || !cursor || !src) {
                if (provider) provider->strtok_next = 0u;
                result->r0 = 0u;
                break;
            }
            {
                uint32_t scanned = 0u;
                for (; scanned < RE_GUEST_LIBC_STRING_MAX_BYTES; ++scanned) {
                    int delim_match;
                    if (!read_byte(bus, cursor, 0u, &a)) return RE_IMPORT_E_HANDLER;
                    if (!a) {
                        provider->strtok_next = 0u;
                        result->r0 = 0u;
                        return RE_IMPORT_OK;
                    }
                    delim_match = is_delimiter(bus, src, a);
                    if (delim_match < 0) return RE_IMPORT_E_HANDLER;
                    if (!delim_match) break;
                    if (!add_address(cursor, 1u, &cursor)) return RE_IMPORT_E_HANDLER;
                }
                if (scanned == RE_GUEST_LIBC_STRING_MAX_BYTES)
                    return RE_IMPORT_E_HANDLER;
            }
            start = cursor;
            {
                uint32_t scanned = 0u;
                for (; scanned < RE_GUEST_LIBC_STRING_MAX_BYTES; ++scanned) {
                    int delim_match;
                    if (!read_byte(bus, cursor, 0u, &a)) return RE_IMPORT_E_HANDLER;
                    if (!a) {
                        provider->strtok_next = 0u;
                        result->r0 = start;
                        return RE_IMPORT_OK;
                    }
                    delim_match = is_delimiter(bus, src, a);
                    if (delim_match < 0) return RE_IMPORT_E_HANDLER;
                    if (delim_match) {
                        if (!write_byte(bus, cursor, 0u, 0u) ||
                            !add_address(cursor, 1u, &provider->strtok_next))
                            return RE_IMPORT_E_HANDLER;
                        result->r0 = start;
                        return RE_IMPORT_OK;
                    }
                    if (!add_address(cursor, 1u, &cursor)) return RE_IMPORT_E_HANDLER;
                }
                return RE_IMPORT_E_HANDLER;
            }
        }
        default:
            return RE_IMPORT_E_HANDLER;
    }
    return RE_IMPORT_OK;
}

static int find_undefined_symbol(
    const REGUESTELF32LINKSYMBOL *symbols, size_t count,
    const char *name, size_t *found) {
    size_t match = count;
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

REGUESTLIBCStringStatus re_guest_libc_string_provider_init(
    REGUESTLIBCStringProvider *provider) {
    if (!provider) return RE_GUEST_LIBC_STRING_E_ARGUMENT;
    provider->strtok_next = 0u;
    return RE_GUEST_LIBC_STRING_OK;
}

REGUESTLIBCStringStatus re_guest_libc_string_provider_bind_elf_symbols(
    REGUESTLIBCStringProvider *provider, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index) {
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[RE_GUEST_LIBC_STRING_SYMBOL_COUNT];
    size_t indices[RE_GUEST_LIBC_STRING_SYMBOL_COUNT];
    uint32_t pointers[RE_GUEST_LIBC_STRING_SYMBOL_COUNT];

    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !imports || !symbols || !exports || !export_count ||
        export_capacity < RE_GUEST_LIBC_STRING_SYMBOL_COUNT)
        return RE_GUEST_LIBC_STRING_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - RE_GUEST_LIBC_STRING_SYMBOL_COUNT)
        return RE_GUEST_LIBC_STRING_E_CAPACITY;
    for (size_t i = 0; i < RE_GUEST_LIBC_STRING_SYMBOL_COUNT; ++i) {
        if (!find_undefined_symbol(symbols, symbol_count, k_names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = indices[i];
            return RE_GUEST_LIBC_STRING_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j)
            if (indices[i] == indices[j]) return RE_GUEST_LIBC_STRING_E_SYMBOL;
    }

    backup = *imports;
    for (size_t i = 0; i < RE_GUEST_LIBC_STRING_SYMBOL_COUNT; ++i) {
        provider->contexts[i].kind = (uint32_t)k_contexts[i].kind;
        provider->contexts[i].provider = provider;
        if (re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
                libc_string_handler, &provider->contexts[i], &pointers[i]) != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_LIBC_STRING_E_IMPORT;
        }
        pending[i] = (REGUESTSYMBOL){
            k_names[i], pointers[i], RE_GUEST_STB_GLOBAL,
            RE_GUEST_STV_DEFAULT, 1u, NULL, "libc.so"
        };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = RE_GUEST_LIBC_STRING_SYMBOL_COUNT;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_LIBC_STRING_OK;
}
