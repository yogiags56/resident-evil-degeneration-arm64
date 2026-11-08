\
#include "re_file_adapter.h"

#include <string.h>

static int guest_range(const REGUESTMEM *m, uint32_t a, size_t n, size_t *offset) {
    uint64_t off;
    if (!m || !m->bytes || a < m->guest_base) return 0;
    off = (uint64_t)a - (uint64_t)m->guest_base;
    if (off > (uint64_t)m->size || (uint64_t)n > (uint64_t)m->size - off) return 0;
    if (offset) *offset = (size_t)off;
    return 1;
}

REFileStatus re_file_guest_span(const REGUESTMEM *m, uint32_t a,
                                size_t n, uint8_t **p) {
    size_t off;
    if (!p) return RE_FILE_E_ARGUMENT;
    *p = NULL;
    if (!guest_range(m, a, n, &off)) return RE_FILE_E_GUEST_MEMORY;
    *p = m->bytes + off;
    return RE_FILE_OK;
}

static REFileStatus guest_string(const REGUESTMEM *m, uint32_t a,
                                 char *dst, size_t capacity) {
    size_t off, i;
    if (!dst || capacity == 0) return RE_FILE_E_ARGUMENT;
    if (!guest_range(m, a, 1, &off)) return RE_FILE_E_GUEST_MEMORY;
    for (i = 0; i < capacity; ++i) {
        uint64_t pos = (uint64_t)off + i;
        if (pos >= (uint64_t)m->size) return RE_FILE_E_GUEST_MEMORY;
        dst[i] = (char)m->bytes[(size_t)pos];
        if (dst[i] == '\0') return RE_FILE_OK;
    }
    dst[capacity - 1] = '\0';
    return RE_FILE_E_PATH;
}

REFileStatus re_file_normalize_path(const char *input,
                                    char output[RE_FILE_ADAPTER_MAX_PATH]) {
    size_t i = 0, o = 0;
    if (!input || !output) return RE_FILE_E_ARGUMENT;
    output[0] = '\0';
    if (input[0] == '/' || input[0] == '\\') return RE_FILE_E_PATH;

    while (input[i] != '\0') {
        size_t start, length;
        while (input[i] == '/' || input[i] == '\\') ++i;
        if (input[i] == '\0') break;
        start = i;
        while (input[i] != '\0' && input[i] != '/' && input[i] != '\\') {
            if (input[i] == ':') return RE_FILE_E_PATH;
            ++i;
        }
        length = i - start;
        if (length == 0) continue;
        if (length == 1 && input[start] == '.') continue;
        if (length == 2 && input[start] == '.' && input[start + 1] == '.') {
            return RE_FILE_E_PATH;
        }
        if (o != 0) {
            if (o + 1 >= RE_FILE_ADAPTER_MAX_PATH) return RE_FILE_E_PATH;
            output[o++] = '/';
        }
        if (length >= RE_FILE_ADAPTER_MAX_PATH - o) return RE_FILE_E_PATH;
        memcpy(output + o, input + start, length);
        o += length;
    }
    if (o == 0) return RE_FILE_E_PATH;
    output[o] = '\0';
    return RE_FILE_OK;
}

REFileStatus re_file_adapter_init(REFileAdapter *a,
                                  const REFileBackend *b, void *user) {
    if (!a || !b || !b->open || !b->read || !b->get_char || !b->close)
        return RE_FILE_E_ARGUMENT;
    memset(a, 0, sizeof(*a));
    a->backend = *b;
    a->backend_user = user;
    a->next_generation = 1;
    return RE_FILE_OK;
}

static uint32_t make_token(size_t index, uint16_t generation) {
    return ((uint32_t)generation << 16) | (uint32_t)(index + 1u);
}

static REFileHandleSlot *resolve_handle(REFileAdapter *a, uint32_t token) {
    uint32_t low = token & 0xffffu;
    uint16_t generation = (uint16_t)(token >> 16);
    size_t index;
    REFileHandleSlot *s;
    if (!a || low == 0 || low > RE_FILE_ADAPTER_MAX_HANDLES || generation == 0)
        return NULL;
    index = (size_t)(low - 1u);
    s = &a->slots[index];
    if (!s->active || s->generation != generation || !s->stream) return NULL;
    return s;
}

REFileStatus re_file_adapter_open(REFileAdapter *a, const REGUESTMEM *m,
                                  uint32_t guest_path, uint32_t guest_mode,
                                  uint32_t *guest_handle) {
    char path[RE_FILE_ADAPTER_MAX_PATH];
    char mode[8];
    size_t i;
    void *stream;
    REFileStatus status;
    uint16_t generation;
    if (!a || !m || !guest_handle) return RE_FILE_E_ARGUMENT;
    *guest_handle = 0;
    status = guest_string(m, guest_path, path, sizeof(path));
    if (status != RE_FILE_OK) return status;
    status = guest_string(m, guest_mode, mode, sizeof(mode));
    if (status != RE_FILE_OK) return status;
    if (strcmp(mode, "rb") != 0) return RE_FILE_E_MODE;
    {
        char normalized[RE_FILE_ADAPTER_MAX_PATH];
        status = re_file_normalize_path(path, normalized);
        if (status != RE_FILE_OK) return status;
        strcpy(path, normalized);
    }
    for (i = 0; i < RE_FILE_ADAPTER_MAX_HANDLES; ++i)
        if (!a->slots[i].active) break;
    if (i == RE_FILE_ADAPTER_MAX_HANDLES) return RE_FILE_E_CAPACITY;

    stream = a->backend.open(a->backend_user, path, "rb");
    if (!stream) return RE_FILE_E_NOT_FOUND;

    generation = a->next_generation++;
    if (generation == 0) generation = a->next_generation++;
    if (a->next_generation == 0) a->next_generation = 1;
    a->slots[i].stream = stream;
    a->slots[i].generation = generation;
    a->slots[i].active = 1;
    *guest_handle = make_token(i, generation);
    return RE_FILE_OK;
}

REFileStatus re_file_adapter_read_bytes(REFileAdapter *a,
                                        const REGUESTMEM *m,
                                        uint32_t token, uint32_t guest_dst,
                                        uint64_t count, uint64_t *read_out) {
    REFileHandleSlot *s;
    uint8_t *dst;
    int64_t got;
    if (read_out) *read_out = 0;
    if (!a || !m || !read_out) return RE_FILE_E_ARGUMENT;
    s = resolve_handle(a, token);
    if (!s) return RE_FILE_E_BAD_HANDLE;
    if (count > (uint64_t)SIZE_MAX || count > UINT32_MAX)
        return RE_FILE_E_OVERFLOW;
    if (re_file_guest_span(m, guest_dst, (size_t)count, &dst) != RE_FILE_OK)
        return RE_FILE_E_GUEST_MEMORY;
    if (count == 0) return RE_FILE_OK;
    got = a->backend.read(a->backend_user, s->stream, dst, count);
    if (got < 0) return RE_FILE_E_BACKEND;
    if ((uint64_t)got > count) return RE_FILE_E_BACKEND;
    *read_out = (uint64_t)got;
    return RE_FILE_OK;
}

REFileStatus re_file_adapter_get_char(REFileAdapter *a, uint32_t token,
                                      int32_t *value) {
    REFileHandleSlot *s;
    int32_t c;
    if (!a || !value) return RE_FILE_E_ARGUMENT;
    s = resolve_handle(a, token);
    if (!s) return RE_FILE_E_BAD_HANDLE;
    c = a->backend.get_char(a->backend_user, s->stream);
    if (c < -1 || c > 255) return RE_FILE_E_BACKEND;
    *value = c;
    return RE_FILE_OK;
}

REFileStatus re_file_adapter_close(REFileAdapter *a, uint32_t token) {
    REFileHandleSlot *s;
    if (!a) return RE_FILE_E_ARGUMENT;
    s = resolve_handle(a, token);
    if (!s) return RE_FILE_E_BAD_HANDLE;
    a->backend.close(a->backend_user, s->stream);
    s->stream = NULL;
    s->active = 0;
    return RE_FILE_OK;
}

void re_file_adapter_close_all(REFileAdapter *a) {
    size_t i;
    if (!a) return;
    for (i = 0; i < RE_FILE_ADAPTER_MAX_HANDLES; ++i) {
        if (a->slots[i].active && a->slots[i].stream) {
            a->backend.close(a->backend_user, a->slots[i].stream);
            a->slots[i].stream = NULL;
            a->slots[i].active = 0;
        }
    }
}
