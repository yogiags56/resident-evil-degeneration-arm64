#include "re_guest_module_manifest.h"
#include <string.h>
#include <limits.h>

static int valid_soname(const char *s) {
    size_t n;
    if (!s || !(n = strlen(s)) || n >= RE_GUEST_MANIFEST_SONAME_CAP ||
        strchr(s, '/') || strchr(s, '\\')) return 0;
    for (size_t i = 0; i < n; ++i)
        if ((unsigned char)s[i] < 0x21u || (unsigned char)s[i] > 0x7eu)
            return 0;
    return 1;
}
static int valid_path(const char *s) {
    size_t n, i = 0;
    if (!s || !(n = strlen(s)) || n >= RE_GUEST_MANIFEST_PATH_CAP ||
        s[0] == '/' || s[0] == '\\') return 0;
    while (i < n) {
        size_t start;
        while (i < n && (s[i] == '/' || s[i] == '\\')) return 0;
        start = i;
        while (i < n && s[i] != '/' && s[i] != '\\') {
            unsigned char c = (unsigned char)s[i];
            if (c < 0x21u || c > 0x7eu || c == ':' ) return 0;
            ++i;
        }
        if (i == start || (i - start == 1u && s[start] == '.') ||
            (i - start == 2u && s[start] == '.' && s[start + 1u] == '.'))
            return 0;
        if (i < n) { ++i; if (i == n) return 0; }
    }
    return 1;
}
static int next_token(const char **cursor, const char *end,
                      char *out, size_t capacity) {
    size_t n = 0;
    while (*cursor < end && (**cursor == ' ' || **cursor == '\t')) ++*cursor;
    if (*cursor == end || **cursor == '#') return 0;
    while (*cursor < end && **cursor != ' ' && **cursor != '\t') {
        if (n + 1u >= capacity) return -1;
        out[n++] = *(*cursor)++;
    }
    out[n] = '\0';
    return n ? 1 : 0;
}
static int parse_limit(const char *s, uint32_t *out) {
    uint64_t value = 0;
    if (!s || !*s || !out) return 0;
    for (size_t i = 0; s[i]; ++i) {
        unsigned digit;
        if (s[i] < '0' || s[i] > '9') return 0;
        digit = (unsigned)(s[i] - '0');
        if (value > (UINT32_MAX - digit) / 10u) return 0;
        value = value * 10u + digit;
    }
    if (!value || value > RE_GUEST_MANIFEST_MAX_MODULE_BYTES) return 0;
    *out = (uint32_t)value;
    return 1;
}
REGUESTMODULEMANIFESTStatus re_guest_module_manifest_parse(
    const char *text, size_t length, REGUESTMODULEMANIFEST *manifest,
    size_t *failed_line) {
    REGUESTMODULEMANIFEST pending;
    size_t line_number = 0, offset = 0, root_count = 0;
    char root[RE_GUEST_MANIFEST_SONAME_CAP] = {0};
    int header_seen = 0;
    if (failed_line) *failed_line = 0;
    if (!text || !manifest) return RE_GUEST_MANIFEST_E_ARGUMENT;
    if (!length || length > RE_GUEST_MANIFEST_MAX_BYTES)
        return RE_GUEST_MANIFEST_E_SIZE;
    memset(&pending, 0, sizeof(pending));
    while (offset < length) {
        size_t start = offset, end;
        const char *cursor;
        char a[32], b[RE_GUEST_MANIFEST_SONAME_CAP];
        char c[RE_GUEST_MANIFEST_PATH_CAP], d[32];
        int ta, tb, tc, td;
        ++line_number;
        while (offset < length && text[offset] != '\n') {
            if (text[offset] == '\0') {
                if (failed_line) *failed_line = line_number;
                return RE_GUEST_MANIFEST_E_SYNTAX;
            }
            ++offset;
        }
        end = offset;
        if (offset < length) ++offset;
        if (end > start && text[end - 1u] == '\r') --end;
        cursor = text + start;
        ta = next_token(&cursor, text + end, a, sizeof(a));
        if (ta == 0) continue;
        if (ta < 0) goto syntax;
        if (!header_seen) {
            tb = next_token(&cursor, text + end, b, sizeof(b));
            tc = next_token(&cursor, text + end, c, sizeof(c));
            if (strcmp(a, "RE-GUEST-MODULES") || tb != 1 ||
                strcmp(b, "1") || tc != 0) {
                if (failed_line) *failed_line = line_number;
                return RE_GUEST_MANIFEST_E_VERSION;
            }
            header_seen = 1;
            pending.version = 1u;
            continue;
        }
        if (!strcmp(a, "root")) {
            tb = next_token(&cursor, text + end, b, sizeof(b));
            tc = next_token(&cursor, text + end, c, sizeof(c));
            if (tb != 1 || tc != 0 || !valid_soname(b)) goto name_error;
            if (++root_count != 1u) goto syntax;
            strcpy(root, b);
            continue;
        }
        if (!strcmp(a, "module")) {
            REGUESTMODULEMANIFESTENTRY *entry;
            tb = next_token(&cursor, text + end, b, sizeof(b));
            tc = next_token(&cursor, text + end, c, sizeof(c));
            td = next_token(&cursor, text + end, d, sizeof(d));
            {
                char extra[2];
                int te = next_token(&cursor, text + end, extra, sizeof(extra));
                if (tb != 1 || tc != 1 || td != 1 || te != 0)
                    goto syntax;
            }
            if (!valid_soname(b)) goto name_error;
            if (!valid_path(c)) goto path_error;
            if (pending.module_count >= RE_GUEST_MANIFEST_MAX_MODULES) {
                if (failed_line) *failed_line = line_number;
                return RE_GUEST_MANIFEST_E_CAPACITY;
            }
            entry = &pending.modules[pending.module_count];
            if (!parse_limit(d, &entry->max_bytes)) {
                if (failed_line) *failed_line = line_number;
                return RE_GUEST_MANIFEST_E_LIMIT;
            }
            strcpy(entry->soname, b);
            strcpy(entry->asset_path, c);
            for (size_t i = 0; i < pending.module_count; ++i) {
                if (!strcmp(pending.modules[i].soname, entry->soname) ||
                    !strcmp(pending.modules[i].asset_path, entry->asset_path)) {
                    if (failed_line) *failed_line = line_number;
                    return RE_GUEST_MANIFEST_E_DUPLICATE;
                }
            }
            ++pending.module_count;
            continue;
        }
        goto syntax;
name_error:
        if (failed_line) *failed_line = line_number;
        return RE_GUEST_MANIFEST_E_NAME;
path_error:
        if (failed_line) *failed_line = line_number;
        return RE_GUEST_MANIFEST_E_PATH;
syntax:
        if (failed_line) *failed_line = line_number;
        return RE_GUEST_MANIFEST_E_SYNTAX;
    }
    if (!header_seen || root_count != 1u || !pending.module_count) {
        if (failed_line) *failed_line = line_number ? line_number : 1u;
        return RE_GUEST_MANIFEST_E_SYNTAX;
    }
    pending.root_module_index = pending.module_count;
    for (size_t i = 0; i < pending.module_count; ++i) {
        if (!strcmp(pending.modules[i].soname, root)) {
            pending.root_module_index = i;
            break;
        }
    }
    if (pending.root_module_index == pending.module_count) {
        if (failed_line) *failed_line = line_number;
        return RE_GUEST_MANIFEST_E_ROOT;
    }
    *manifest = pending;
    return RE_GUEST_MANIFEST_OK;
}
