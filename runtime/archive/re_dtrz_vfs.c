#include "re_dtrz_vfs.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static uint16_t read_u16le(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int range_ok(size_t total, uint64_t offset, uint64_t length) {
    return offset <= (uint64_t)total && length <= (uint64_t)total - offset;
}

static int valid_component(const char *s, size_t n) {
    size_t i;
    if (n == 0 || (n == 1 && s[0] == '.') ||
        (n == 2 && s[0] == '.' && s[1] == '.')) return 0;
    for (i = 0; i < n; ++i) {
        if (s[i] == '/' || s[i] == '\\' || s[i] == ':') return 0;
    }
    return 1;
}

static int normalize_archive_folder(const char *raw, char *dst, size_t cap) {
    size_t i = 0, out = 0, start = 0, n;
    if (!raw || !dst || cap == 0 || raw[0] == '\0') return 0;
    n = strlen(raw);
    if (raw[0] == '/' || raw[0] == '\\') return 0;
    while (i <= n) {
        if (i == n || raw[i] == '/' || raw[i] == '\\') {
            size_t part_len = i - start;
            if (!valid_component(raw + start, part_len)) return 0;
            if (out != 0) {
                if (out + 1 >= cap) return 0;
                dst[out++] = '/';
            }
            if (part_len >= cap - out) return 0;
            memcpy(dst + out, raw + start, part_len);
            out += part_len;
            start = i + 1;
        }
        ++i;
    }
    if (out >= cap) return 0;
    dst[out] = '\0';
    return 1;
}

static char *copy_cstring(const uint8_t *bytes, size_t total, size_t *cursor) {
    size_t start, end, length;
    char *result;
    if (!bytes || !cursor || *cursor >= total) return NULL;
    start = *cursor;
    end = start;
    while (end < total && bytes[end] != 0) ++end;
    if (end == total) return NULL;
    length = end - start;
    result = (char *)malloc(length + 1u);
    if (!result) return NULL;
    memcpy(result, bytes + start, length);
    result[length] = '\0';
    *cursor = end + 1u;
    return result;
}

void re_dtrz_archive_destroy(re_dtrz_archive *archive) {
    uint32_t i;
    if (!archive) return;
    if (archive->entries) {
        for (i = 0; i < archive->file_count; ++i) free(archive->entries[i].path);
        free(archive->entries);
    }
    memset(archive, 0, sizeof(*archive));
}

re_dtrz_status re_dtrz_archive_parse(
    re_dtrz_archive *out, const uint8_t *bytes, size_t byte_count) {
    uint32_t file_count, folder_count, i;
    size_t cursor, attribute_offset, attribute_end, location_header_offset;
    size_t location_table_offset, payload_offset;
    char **filenames = NULL;
    char **folders = NULL;
    uint16_t *folder_indices = NULL;
    uint16_t *file_indices = NULL;
    uint16_t *flags = NULL;
    uint32_t *offsets = NULL;
    re_dtrz_archive temp;
    re_dtrz_status status = RE_DTRZ_E_FORMAT;

    if (!out || !bytes) return RE_DTRZ_E_ARGUMENT;
    memset(out, 0, sizeof(*out));
    memset(&temp, 0, sizeof(temp));
    if (byte_count < RE_DTRZ_HEADER_SIZE || byte_count > UINT32_MAX ||
        memcmp(bytes, RE_DTRZ_MAGIC, 4) != 0)
        return RE_DTRZ_E_FORMAT;

    file_count = read_u16le(bytes + 4);
    folder_count = read_u16le(bytes + 6);
    if (file_count == 0 || folder_count == 0 || bytes[8] != 0)
        return RE_DTRZ_E_FORMAT;

    filenames = (char **)calloc(file_count, sizeof(char *));
    folders = (char **)calloc(folder_count, sizeof(char *));
    folder_indices = (uint16_t *)calloc(file_count, sizeof(uint16_t));
    file_indices = (uint16_t *)calloc(file_count, sizeof(uint16_t));
    flags = (uint16_t *)calloc(file_count, sizeof(uint16_t));
    offsets = (uint32_t *)calloc(file_count, sizeof(uint32_t));
    temp.entries = (re_dtrz_entry *)calloc(file_count, sizeof(re_dtrz_entry));
    if (!filenames || !folders || !folder_indices || !file_indices || !flags ||
        !offsets || !temp.entries) { status = RE_DTRZ_E_MEMORY; goto cleanup; }
    temp.file_count = file_count;
    temp.folder_count = folder_count;

    cursor = RE_DTRZ_HEADER_SIZE;
    for (i = 0; i < file_count; ++i) {
        filenames[i] = copy_cstring(bytes, byte_count, &cursor);
        if (!filenames[i]) { status = RE_DTRZ_E_FORMAT; goto cleanup; }
        if (!valid_component(filenames[i], strlen(filenames[i]))) {
            status = RE_DTRZ_E_FORMAT; goto cleanup;
        }
    }
    for (i = 1; i < folder_count; ++i) {
        folders[i] = copy_cstring(bytes, byte_count, &cursor);
        if (!folders[i]) { status = RE_DTRZ_E_FORMAT; goto cleanup; }
        {
            char normalized[RE_DTRZ_PATH_CAPACITY];
            if (!normalize_archive_folder(folders[i], normalized, sizeof(normalized))) {
                status = RE_DTRZ_E_FORMAT; goto cleanup;
            }
            free(folders[i]);
            folders[i] = (char *)malloc(strlen(normalized) + 1u);
            if (!folders[i]) { status = RE_DTRZ_E_MEMORY; goto cleanup; }
            strcpy(folders[i], normalized);
        }
    }

    attribute_offset = cursor;
    if (!range_ok(byte_count, attribute_offset, (uint64_t)file_count * RE_DTRZ_ATTRIBUTE_SIZE)) {
        status = RE_DTRZ_E_BOUNDS; goto cleanup;
    }
    attribute_end = attribute_offset + (size_t)file_count * RE_DTRZ_ATTRIBUTE_SIZE;
    for (i = 0; i < file_count; ++i) {
        const uint8_t *p = bytes + attribute_offset + (size_t)i * RE_DTRZ_ATTRIBUTE_SIZE;
        folder_indices[i] = read_u16le(p);
        file_indices[i] = read_u16le(p + 2);
        flags[i] = read_u16le(p + 4);
        if (folder_indices[i] >= folder_count || file_indices[i] != i) {
            status = RE_DTRZ_E_FORMAT; goto cleanup;
        }
    }

    location_header_offset = attribute_end;
    if (!range_ok(byte_count, location_header_offset, RE_DTRZ_LOCATION_HEADER_SIZE)) {
        status = RE_DTRZ_E_BOUNDS; goto cleanup;
    }
    if (read_u16le(bytes + location_header_offset + 2) != file_count) {
        status = RE_DTRZ_E_FORMAT; goto cleanup;
    }
    location_table_offset = location_header_offset + RE_DTRZ_LOCATION_HEADER_SIZE;
    if (!range_ok(byte_count, location_table_offset,
                  (uint64_t)file_count * RE_DTRZ_LOCATION_RECORD_SIZE)) {
        status = RE_DTRZ_E_BOUNDS; goto cleanup;
    }
    payload_offset = location_table_offset + (size_t)file_count * RE_DTRZ_LOCATION_RECORD_SIZE;

    for (i = 0; i < file_count; ++i) {
        const uint8_t *p = bytes + location_table_offset + (size_t)i * RE_DTRZ_LOCATION_RECORD_SIZE;
        offsets[i] = read_u32le(p);
        if ((i == 0 && offsets[i] != payload_offset) ||
            (i != 0 && offsets[i] <= offsets[i - 1]) || offsets[i] >= byte_count) {
            status = RE_DTRZ_E_FORMAT; goto cleanup;
        }
    }

    for (i = 0; i < file_count; ++i) {
        const uint8_t *p = bytes + location_table_offset + (size_t)i * RE_DTRZ_LOCATION_RECORD_SIZE;
        uint32_t end = (i + 1u < file_count) ? offsets[i + 1u] : (uint32_t)byte_count;
        uint16_t folder_index = folder_indices[i];
        size_t folder_len = folder_index == 0 ? 0u : strlen(folders[folder_index]);
        size_t name_len = strlen(filenames[i]);
        size_t path_len = folder_len + (folder_len ? 1u : 0u) + name_len;
        re_dtrz_entry *entry = &temp.entries[i];
        if (end <= offsets[i] || path_len >= RE_DTRZ_PATH_CAPACITY) {
            status = RE_DTRZ_E_FORMAT; goto cleanup;
        }
        entry->path = (char *)malloc(path_len + 1u);
        if (!entry->path) { status = RE_DTRZ_E_MEMORY; goto cleanup; }
        if (folder_len) {
            memcpy(entry->path, folders[folder_index], folder_len);
            entry->path[folder_len] = '/';
            memcpy(entry->path + folder_len + 1u, filenames[i], name_len + 1u);
        } else {
            memcpy(entry->path, filenames[i], name_len + 1u);
        }
        entry->index = i;
        entry->folder_index = folder_index;
        entry->file_index = file_indices[i];
        entry->flags = flags[i];
        entry->offset = offsets[i];
        entry->packed_size = (uint64_t)end - offsets[i];
        entry->decoded_size_a = read_u32le(p + 4);
        entry->decoded_size_b = read_u32le(p + 8);
        entry->method = read_u32le(p + 12);
    }

    temp.bytes = bytes;
    temp.byte_count = byte_count;
    temp.file_count = file_count;
    temp.folder_count = folder_count;
    temp.location_header_unknown = read_u16le(bytes + location_header_offset);
    temp.attribute_offset = attribute_offset;
    temp.location_header_offset = location_header_offset;
    temp.location_table_offset = location_table_offset;
    temp.payload_offset = payload_offset;
    *out = temp;
    memset(&temp, 0, sizeof(temp));
    status = RE_DTRZ_OK;

cleanup:
    if (filenames) for (i = 0; i < file_count; ++i) free(filenames[i]);
    if (folders) for (i = 1; i < folder_count; ++i) free(folders[i]);
    free(filenames); free(folders); free(folder_indices); free(file_indices); free(flags); free(offsets);
    re_dtrz_archive_destroy(&temp);
    return status;
}

const re_dtrz_entry *re_dtrz_find(const re_dtrz_archive *archive, const char *path) {
    uint32_t i;
    char normalized[RE_DTRZ_PATH_CAPACITY];
    size_t n, out = 0, start = 0, j;
    if (!archive || !archive->entries || !path) return NULL;
    n = strlen(path);
    if (n == 0 || n >= sizeof(normalized) || path[0] == '/' || path[0] == '\\') return NULL;
    for (j = 0; j <= n; ++j) {
        if (j == n || path[j] == '/' || path[j] == '\\') {
            size_t len = j - start;
            if (!valid_component(path + start, len)) return NULL;
            if (out && out + 1u >= sizeof(normalized)) return NULL;
            if (out) normalized[out++] = '/';
            if (len >= sizeof(normalized) - out) return NULL;
            memcpy(normalized + out, path + start, len);
            out += len;
            start = j + 1u;
        }
    }
    normalized[out] = '\0';
    for (i = 0; i < archive->file_count; ++i) {
        if (strcmp(archive->entries[i].path, normalized) == 0) return &archive->entries[i];
    }
    return NULL;
}

re_dtrz_status re_dtrz_vfs_init(
    re_dtrz_vfs *vfs, const re_dtrz_archive *archive,
    re_dtrz_decode_fn decode, void *decode_context) {
    if (!vfs || !archive || !archive->entries || !archive->bytes) return RE_DTRZ_E_ARGUMENT;
    vfs->archive = archive;
    vfs->decode = decode;
    vfs->decode_context = decode_context;
    return RE_DTRZ_OK;
}

re_dtrz_status re_dtrz_open(
    const re_dtrz_vfs *vfs, const char *path, re_dtrz_file *out_file) {
    const re_dtrz_entry *entry;
    uint8_t *decoded;
    size_t written = 0;
    re_dtrz_status status;
    if (!vfs || !vfs->archive || !out_file) return RE_DTRZ_E_ARGUMENT;
    memset(out_file, 0, sizeof(*out_file));
    entry = re_dtrz_find(vfs->archive, path);
    if (!entry) return RE_DTRZ_E_NOT_FOUND;
    if (entry->decoded_size_a != entry->decoded_size_b) return RE_DTRZ_E_FORMAT;
    if (entry->decoded_size_a == 0 ||
        entry->decoded_size_a > RE_DTRZ_MAX_DECODED_ENTRY_SIZE) return RE_DTRZ_E_BOUNDS;
    if (!range_ok(vfs->archive->byte_count, entry->offset, entry->packed_size)) return RE_DTRZ_E_BOUNDS;
    decoded = (uint8_t *)malloc(entry->decoded_size_a);
    if (!decoded) return RE_DTRZ_E_MEMORY;

    if (entry->method == RE_DTRZ_METHOD_STORED) {
        if (entry->packed_size != entry->decoded_size_a) {
            free(decoded); return RE_DTRZ_E_FORMAT;
        }
        memcpy(decoded, vfs->archive->bytes + (size_t)entry->offset, entry->decoded_size_a);
        written = entry->decoded_size_a;
    } else {
        if (!vfs->decode) { free(decoded); return RE_DTRZ_E_UNSUPPORTED; }
        status = vfs->decode(vfs->decode_context, entry->method,
            vfs->archive->bytes + (size_t)entry->offset, (size_t)entry->packed_size,
            decoded, entry->decoded_size_a, &written);
        if (status != RE_DTRZ_OK) { free(decoded); return status == RE_DTRZ_E_UNSUPPORTED ? status : RE_DTRZ_E_DECODE; }
        if (written != entry->decoded_size_a) { free(decoded); return RE_DTRZ_E_DECODE; }
    }
    out_file->decoded = decoded;
    out_file->size = written;
    out_file->cursor = 0;
    out_file->is_open = 1;
    return RE_DTRZ_OK;
}

re_dtrz_status re_dtrz_read(
    re_dtrz_file *file, void *destination, size_t requested, size_t *read_count) {
    size_t remaining, amount;
    if (!file || !read_count || (!destination && requested != 0)) return RE_DTRZ_E_ARGUMENT;
    *read_count = 0;
    if (!file->is_open || !file->decoded) return RE_DTRZ_E_CLOSED;
    remaining = file->size - file->cursor;
    amount = requested < remaining ? requested : remaining;
    if (amount) memcpy(destination, file->decoded + file->cursor, amount);
    file->cursor += amount;
    *read_count = amount;
    return RE_DTRZ_OK;
}

re_dtrz_status re_dtrz_seek(re_dtrz_file *file, uint64_t offset) {
    if (!file) return RE_DTRZ_E_ARGUMENT;
    if (!file->is_open || !file->decoded) return RE_DTRZ_E_CLOSED;
    if (offset > file->size) return RE_DTRZ_E_BOUNDS;
    file->cursor = (size_t)offset;
    return RE_DTRZ_OK;
}

re_dtrz_status re_dtrz_tell(const re_dtrz_file *file, uint64_t *offset) {
    if (!file || !offset) return RE_DTRZ_E_ARGUMENT;
    if (!file->is_open || !file->decoded) return RE_DTRZ_E_CLOSED;
    *offset = file->cursor;
    return RE_DTRZ_OK;
}

void re_dtrz_close(re_dtrz_file *file) {
    if (!file) return;
    free(file->decoded);
    memset(file, 0, sizeof(*file));
}

const char *re_dtrz_status_string(re_dtrz_status status) {
    switch (status) {
        case RE_DTRZ_OK: return "ok";
        case RE_DTRZ_E_ARGUMENT: return "invalid argument";
        case RE_DTRZ_E_FORMAT: return "invalid archive format";
        case RE_DTRZ_E_BOUNDS: return "out of bounds";
        case RE_DTRZ_E_MEMORY: return "out of memory";
        case RE_DTRZ_E_NOT_FOUND: return "entry not found";
        case RE_DTRZ_E_UNSUPPORTED: return "compression method unsupported";
        case RE_DTRZ_E_DECODE: return "payload decode failed";
        case RE_DTRZ_E_CLOSED: return "file is closed";
        default: return "unknown status";
    }
}
