#include "re_guest_module_materializer.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static int safe_relative_asset_path(const char *path) {
    size_t n, i = 0u;
    if (!path || !(n = strlen(path)) ||
        n >= RE_GUEST_MANIFEST_PATH_CAP || path[0] == '/' || path[0] == '\\')
        return 0;
    while (i < n) {
        size_t start = i;
        while (i < n && path[i] != '/' && path[i] != '\\') {
            unsigned char c = (unsigned char)path[i];
            if (c < 0x21u || c > 0x7eu || c == ':') return 0;
            ++i;
        }
        if (i == start || (i-start == 1u && path[start] == '.') ||
            (i-start == 2u && path[start] == '.' && path[start+1u] == '.'))
            return 0;
        if (i < n) { ++i; if (i == n || path[i] == '/' || path[i] == '\\') return 0; }
    }
    return 1;
}
static int output_is_empty(const REGUESTMATERIALIZEDSET *set) {
    if (!set || set->module_count != 0u) return 0;
    for (size_t i = 0; i < RE_GUEST_MANIFEST_MAX_MODULES; ++i)
        if (set->modules[i].bytes || set->modules[i].size) return 0;
    return 1;
}
void re_guest_module_materialized_free(REGUESTMATERIALIZEDSET *set) {
    if (!set) return;
    for (size_t i = 0; i < RE_GUEST_MANIFEST_MAX_MODULES; ++i) {
        free(set->modules[i].bytes);
        set->modules[i].bytes = NULL;
        set->modules[i].size = 0u;
    }
    set->module_count = 0u;
}
REGUESTMATERIALIZESTATUS re_guest_module_materialize(
    const REGUESTMODULEMANIFEST *manifest, const REGUESTMODULESOURCE *source,
    void *source_user, REGUESTMATERIALIZEDSET *out) {
    REGUESTMATERIALIZEDSET pending;
    uint64_t total = 0u;
    if (!manifest || !source || !out || !source->open || !source->length ||
        !source->read || !source->close || !output_is_empty(out))
        return RE_GUEST_MATERIALIZE_E_ARGUMENT;
    if (manifest->version != 1u || manifest->module_count == 0u ||
        manifest->module_count > RE_GUEST_MANIFEST_MAX_MODULES ||
        manifest->root_module_index >= manifest->module_count)
        return RE_GUEST_MATERIALIZE_E_MANIFEST;
    memset(&pending, 0, sizeof(pending));
    for (size_t i = 0; i < manifest->module_count; ++i) {
        const REGUESTMODULEMANIFESTENTRY *entry = &manifest->modules[i];
        void *stream;
        int64_t signed_length;
        uint64_t length, offset = 0u;
        uint8_t *buffer;
        if (!memchr(entry->asset_path, '\0', sizeof(entry->asset_path)) ||
            !memchr(entry->soname, '\0', sizeof(entry->soname)) ||
            !safe_relative_asset_path(entry->asset_path) ||
            entry->max_bytes == 0u ||
            entry->max_bytes > RE_GUEST_MANIFEST_MAX_MODULE_BYTES)
            goto manifest_error;
        stream = source->open(source_user, entry->asset_path);
        if (!stream) goto open_error;
        signed_length = source->length(source_user, stream);
        if (signed_length <= 0) {
            source->close(source_user, stream);
            goto length_error;
        }
        length = (uint64_t)signed_length;
        if (length > entry->max_bytes) {
            source->close(source_user, stream);
            goto limit_error;
        }
        if (length > RE_GUEST_MATERIALIZER_MAX_TOTAL_BYTES - total) {
            source->close(source_user, stream);
            goto total_limit_error;
        }
        if (length > (uint64_t)SIZE_MAX) {
            source->close(source_user, stream);
            goto limit_error;
        }
        buffer = (uint8_t *)malloc((size_t)length);
        if (!buffer) {
            source->close(source_user, stream);
            goto memory_error;
        }
        while (offset < length) {
            uint64_t chunk = length - offset;
            int64_t got;
            if (chunk > 1024u * 1024u) chunk = 1024u * 1024u;
            got = source->read(source_user, stream, buffer + (size_t)offset, chunk);
            if (got <= 0 || (uint64_t)got > chunk) {
                free(buffer);
                source->close(source_user, stream);
                goto read_error;
            }
            offset += (uint64_t)got;
        }
        source->close(source_user, stream);
        pending.modules[i].bytes = buffer;
        pending.modules[i].size = (size_t)length;
        pending.module_count = i + 1u;
        total += length;
    }
    *out = pending;
    return RE_GUEST_MATERIALIZE_OK;
manifest_error:
    re_guest_module_materialized_free(&pending);
    return RE_GUEST_MATERIALIZE_E_MANIFEST;
open_error:
    re_guest_module_materialized_free(&pending);
    return RE_GUEST_MATERIALIZE_E_OPEN;
length_error:
    re_guest_module_materialized_free(&pending);
    return RE_GUEST_MATERIALIZE_E_LENGTH;
limit_error:
    re_guest_module_materialized_free(&pending);
    return RE_GUEST_MATERIALIZE_E_LIMIT;
total_limit_error:
    re_guest_module_materialized_free(&pending);
    return RE_GUEST_MATERIALIZE_E_TOTAL_LIMIT;
memory_error:
    re_guest_module_materialized_free(&pending);
    return RE_GUEST_MATERIALIZE_E_MEMORY;
read_error:
    re_guest_module_materialized_free(&pending);
    return RE_GUEST_MATERIALIZE_E_READ;
}
