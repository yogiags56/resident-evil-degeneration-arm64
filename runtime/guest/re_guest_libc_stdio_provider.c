#include "re_guest_libc_stdio_provider.h"

#include <limits.h>
#include <string.h>

#include "re_guest_format.h"
#include "re_guest_scan.h"

enum { STDIO_FOPEN, STDIO_FCLOSE, STDIO_FREAD, STDIO_FWRITE,
       STDIO_FSEEK, STDIO_FTELL, STDIO_FGETS, STDIO_FPRINTF,
       STDIO_FEOF, STDIO_FERROR, STDIO_FFLUSH, STDIO_SSCANF, STDIO_FSCANF };
static const char *const k_names[RE_GUEST_LIBC_STDIO_SYMBOL_COUNT] = {
    "fopen", "fclose", "fread", "fwrite", "fseek", "ftell", "fgets",
    "fprintf", "feof", "ferror", "fflush", "sscanf", "fscanf"
};

static int guest_cstring(const REGUESTMEMBUS *bus, uint32_t address,
                         char *out, size_t capacity) {
    if (!bus || !bus->memory || !address || !out || capacity == 0u) return 0;
    for (size_t i = 0; i < capacity; ++i) {
        uint64_t at = (uint64_t)address + i;
        uint8_t byte;
        if (at > UINT32_MAX ||
            re_guest_memory_read8(bus, (uint32_t)at, &byte) != RE_GUEST_MEM_OK)
            return 0;
        out[i] = (char)byte;
        if (!byte) return 1;
    }
    return 0;
}

static int normalize_path(const char *input, char output[RE_GUEST_LIBC_STDIO_MAX_PATH]) {
    size_t i = 0u, out = 0u;
    if (!input || !output || !input[0] ||
        input[0] == '/' || input[0] == '\\') return 0;
    while (input[i]) {
        size_t start, length;
        while (input[i] == '/' || input[i] == '\\') ++i;
        if (!input[i]) break;
        start = i;
        while (input[i] && input[i] != '/' && input[i] != '\\') {
            if (input[i] == ':') return 0;
            ++i;
        }
        length = i - start;
        if (!length || (length == 1u && input[start] == '.')) continue;
        if (length == 2u && input[start] == '.' && input[start + 1u] == '.')
            return 0;
        if (out) {
            if (out + 1u >= RE_GUEST_LIBC_STDIO_MAX_PATH) return 0;
            output[out++] = '/';
        }
        if (length >= RE_GUEST_LIBC_STDIO_MAX_PATH - out) return 0;
        memcpy(output + out, input + start, length);
        out += length;
    }
    if (!out) return 0;
    output[out] = '\0';
    return 1;
}

static int parse_mode(const char *mode, uint8_t *readable,
                      uint8_t *writable, uint8_t *append) {
    size_t n;
    int plus = 0, binary = 0;
    if (!mode || !mode[0]) return 0;
    n = strlen(mode);
    if (n > 3u) return 0;
    for (size_t i = 1; i < n; ++i) {
        if (mode[i] == '+' && !plus) plus = 1;
        else if (mode[i] == 'b' && !binary) binary = 1;
        else return 0;
    }
    if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') return 0;
    *readable = (uint8_t)(mode[0] == 'r' || plus);
    *writable = (uint8_t)(mode[0] != 'r' || plus);
    *append = (uint8_t)(mode[0] == 'a');
    return 1;
}

static uint32_t file_token(size_t index, uint16_t generation) {
    return RE_GUEST_LIBC_STDIO_TOKEN_BASE |
           ((uint32_t)generation << 5) | (uint32_t)(index + 1u);
}

static int64_t file_read(REGUESTLIBCStdioProvider *p,
                         REGUESTLIBCStdioFile *f, void *dst,
                         uint64_t bytes) {
    uint8_t *out = (uint8_t *)dst;
    uint64_t copied = 0u;
    if (!p || !f || (!dst && bytes)) return -1;
    if (bytes == 0u) return 0;
    if (f->buffered_size) {
        size_t take = bytes < f->buffered_size ?
                      (size_t)bytes : f->buffered_size;
        memcpy(out, f->buffered, take);
        memmove(f->buffered, f->buffered + take,
                (size_t)f->buffered_size - take);
        f->buffered_size = (uint16_t)(f->buffered_size - take);
        copied = take;
    }
    if (copied < bytes) {
        int64_t got = p->backend.read(p->backend_user, f->stream,
                                      out + copied, bytes - copied);
        if (got < 0 || (uint64_t)got > bytes - copied) return -1;
        copied += (uint64_t)got;
    }
    if (copied < bytes) f->eof = 1u;
    return (int64_t)copied;
}

static REGUESTLIBCStdioFile *resolve_file(REGUESTLIBCStdioProvider *p,
                                           uint32_t token) {
    uint32_t low, generation;
    size_t index;
    REGUESTLIBCStdioFile *f;
    if ((token & 0xffff0000u) != RE_GUEST_LIBC_STDIO_TOKEN_BASE) return NULL;
    low = token & 0x1fu;
    generation = (token >> 5) & 0x7ffu;
    if (!low || low > RE_GUEST_LIBC_STDIO_MAX_FILES || !generation) return NULL;
    index = low - 1u;
    f = &p->files[index];
    if (!f->active || !f->stream || f->generation != generation) return NULL;
    return f;
}

static REGUESTIMPORTStatus stdio_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLIBCStdioContext *ctx = (REGUESTLIBCStdioContext *)user;
    REGUESTLIBCStdioProvider *p;
    REGUESTLIBCStdioFile *f;
    uint32_t a, b, c, d;
    if (!frame || !bus || !bus->memory || !ctx || !ctx->provider || !result)
        return RE_IMPORT_E_HANDLER;
    p = ctx->provider;
    if (!p->initialized) return RE_IMPORT_E_HANDLER;
    a = frame->r[0]; b = frame->r[1]; c = frame->r[2]; d = frame->r[3];
    result->r0 = result->r1 = result->r2 = result->r3 = 0u;

    switch (ctx->kind) {
    case STDIO_FOPEN: {
        char path[RE_GUEST_LIBC_STDIO_MAX_PATH];
        char normalized[RE_GUEST_LIBC_STDIO_MAX_PATH];
        char mode[RE_GUEST_LIBC_STDIO_MAX_MODE];
        uint8_t rd, wr, ap;
        size_t i;
        void *stream;
        if (!guest_cstring(bus, a, path, sizeof(path)) ||
            !guest_cstring(bus, b, mode, sizeof(mode)) ||
            !normalize_path(path, normalized) ||
            !parse_mode(mode, &rd, &wr, &ap)) return RE_IMPORT_E_HANDLER;
        for (i = 0; i < RE_GUEST_LIBC_STDIO_MAX_FILES; ++i)
            if (!p->files[i].active) break;
        if (i == RE_GUEST_LIBC_STDIO_MAX_FILES) return RE_IMPORT_E_HANDLER;
        stream = p->backend.open(p->backend_user, normalized, mode);
        if (!stream) return RE_IMPORT_OK;
        {
            uint16_t generation = (uint16_t)(p->next_generation++ & 0x7ffu);
            if (!generation) generation = (uint16_t)(p->next_generation++ & 0x7ffu);
            if (!p->next_generation) p->next_generation = 1u;
            p->files[i].stream = stream;
            p->files[i].generation = generation;
            p->files[i].active = 1u;
            p->files[i].readable = rd;
            p->files[i].writable = wr;
            p->files[i].append = ap;
            result->r0 = file_token(i, generation);
        }
        break;
    }
    case STDIO_FCLOSE:
        f = resolve_file(p, a);
        if (!f) return RE_IMPORT_E_HANDLER;
        {
            int flush_rc = 0;
            int close_rc;
            if (f->writable && p->backend.flush)
                flush_rc = p->backend.flush(p->backend_user, f->stream);
            if (flush_rc != 0) f->error = 1u;
            close_rc = p->backend.close(p->backend_user, f->stream);
            f->stream = NULL; f->active = 0u;
            result->r0 = (uint32_t)(flush_rc == 0 && close_rc == 0 ? 0 : -1);
        }
        break;
    case STDIO_FREAD:
    case STDIO_FWRITE: {
        uint64_t total;
        int64_t transferred;
        void *host = NULL;
        f = resolve_file(p, d);
        if (!f || (ctx->kind == STDIO_FREAD && !f->readable) ||
            (ctx->kind == STDIO_FWRITE && !f->writable)) return RE_IMPORT_E_HANDLER;
        if (!b || !c) break;
        total = (uint64_t)b * c;
        if (total > UINT32_MAX ||
            re_guest_memory_resolve(bus->memory, a, (size_t)total,
                ctx->kind == STDIO_FREAD ? RE_GUEST_MEM_WRITE : RE_GUEST_MEM_READ,
                &host) != RE_GUEST_MEM_OK) return RE_IMPORT_E_HANDLER;
        if (ctx->kind == STDIO_FREAD)
            transferred = file_read(p, f, host, total);
        else
            transferred = p->backend.write(p->backend_user, f->stream, host, total);
        if (transferred < 0 || (uint64_t)transferred > total) {
            f->error = 1u;
            return RE_IMPORT_E_HANDLER;
        }
        if (ctx->kind == STDIO_FREAD && (uint64_t)transferred < total)
            f->eof = 1u;
        result->r0 = (uint32_t)((uint64_t)transferred / b);
        break;
    }
    case STDIO_FSEEK: {
        int64_t offset = (int32_t)b;
        f = resolve_file(p, a);
        if (!f || !p->backend.seek) {
            result->r0 = (uint32_t)-1;
        } else {
            if ((int32_t)c == 1) offset -= (int64_t)f->buffered_size;
            if (p->backend.seek(p->backend_user, f->stream, offset,
                                (int32_t)c) != 0)
                result->r0 = (uint32_t)-1;
            else { f->eof = 0u; f->buffered_size = 0u; }
        }
        break;
    }
    case STDIO_FTELL: {
        int64_t position;
        f = resolve_file(p, a);
        if (!f) return RE_IMPORT_E_HANDLER;
        if (!p->backend.tell) { result->r0 = (uint32_t)-1; break; }
        position = p->backend.tell(p->backend_user, f->stream);
        if (position < (int64_t)f->buffered_size || position > INT32_MAX)
            result->r0 = (uint32_t)-1;
        else result->r0 = (uint32_t)(position - f->buffered_size);
        break;
    }
    case STDIO_FGETS: {
        uint32_t count = b;
        uint32_t written = 0u;
        void *host = NULL;
        uint8_t byte;
        f = resolve_file(p, c);
        if (!f || !f->readable) return RE_IMPORT_E_HANDLER;
        if (!count) break;
        if (!a || re_guest_memory_resolve(bus->memory, a, count,
                RE_GUEST_MEM_WRITE, &host) != RE_GUEST_MEM_OK)
            return RE_IMPORT_E_HANDLER;
        if (count == 1u) {
            ((char *)host)[0] = '\0';
            result->r0 = a;
            break;
        }
        while (written + 1u < count) {
            int64_t got = file_read(p, f, &byte, 1u);
            if (got < 0 || got > 1) { f->error = 1u; return RE_IMPORT_E_HANDLER; }
            if (got == 0) { f->eof = 1u; break; }
            ((char *)host)[written++] = (char)byte;
            if (byte == (uint8_t)'\n') break;
        }
        if (!written) result->r0 = 0u;
        else {
            ((char *)host)[written] = '\0';
            result->r0 = a;
        }
        break;
    }
    case STDIO_FPRINTF: {
        char format[RE_GUEST_FORMAT_STRING_CAP];
        char output[RE_GUEST_FORMAT_OUTPUT_CAP];
        size_t length = 0u;
        int64_t written;
        f = resolve_file(p, a);
        if (!f || !f->writable) return RE_IMPORT_E_HANDLER;
        if (!guest_cstring(bus, b, format, sizeof(format)) ||
            !re_guest_format_guest_printf(frame, bus, 2u, format,
                output, sizeof(output), &length)) {
            f->error = 1u;
            return RE_IMPORT_E_HANDLER;
        }
        if (length) {
            written = p->backend.write(p->backend_user, f->stream, output, length);
            if (written < 0 || (uint64_t)written != length) {
                f->error = 1u;
                return RE_IMPORT_E_HANDLER;
            }
        }
        result->r0 = (uint32_t)length;
        break;
    }
    case STDIO_SSCANF:
    case STDIO_FSCANF: {
        char input[RE_GUEST_FORMAT_STRING_CAP];
        char format[RE_GUEST_FORMAT_STRING_CAP];
        uint32_t assignments = 0u;
        size_t consumed = 0u;
        int input_failure = 0;
        int ok;
        f = NULL;
        if (!guest_cstring(bus, b, format, sizeof(format)))
            return RE_IMPORT_E_HANDLER;
        if (ctx->kind == STDIO_SSCANF) {
            if (!guest_cstring(bus, a, input, sizeof(input)))
                return RE_IMPORT_E_HANDLER;
        } else {
            f = resolve_file(p, a);
            if (!f || !f->readable || sizeof(input) >
                RE_GUEST_LIBC_STDIO_BUFFER_CAP) return RE_IMPORT_E_HANDLER;
            while (f->buffered_size < sizeof(input) - 1u) {
                int64_t got = p->backend.read(p->backend_user, f->stream,
                    f->buffered + f->buffered_size,
                    sizeof(input) - 1u - f->buffered_size);
                if (got < 0 || (uint64_t)got >
                    sizeof(input) - 1u - f->buffered_size) {
                    f->error = 1u;
                    return RE_IMPORT_E_HANDLER;
                }
                if (got == 0) break;
                f->buffered_size = (uint16_t)(f->buffered_size + got);
            }
            memcpy(input, f->buffered, f->buffered_size);
            input[f->buffered_size] = '\0';
        }
        ok = re_guest_scan_guest_sscanf(frame, (REGUESTMEMBUS *)bus, 2u,
                input, format, &assignments, &consumed, &input_failure);
        if (!ok) {
            if (f) f->error = 1u;
            return RE_IMPORT_E_HANDLER;
        }
        if (f) {
            if (consumed > f->buffered_size) return RE_IMPORT_E_HANDLER;
            memmove(f->buffered, f->buffered + consumed,
                    (size_t)f->buffered_size - consumed);
            f->buffered_size = (uint16_t)(f->buffered_size - consumed);
            f->eof = input_failure ? 1u : 0u;
        }
        result->r0 = assignments ? assignments :
                     (input_failure ? (uint32_t)-1 : 0u);
        break;
    }
    case STDIO_FEOF:
    case STDIO_FERROR:
        f = resolve_file(p, a);
        if (!f) return RE_IMPORT_E_HANDLER;
        result->r0 = ctx->kind == STDIO_FEOF ? (uint32_t)f->eof : (uint32_t)f->error;
        break;
    case STDIO_FFLUSH:
        if (a == 0u) {
            if (p->backend.flush) {
                for (size_t i = 0; i < RE_GUEST_LIBC_STDIO_MAX_FILES; ++i) {
                    REGUESTLIBCStdioFile *open_file = &p->files[i];
                    if (open_file->active && open_file->stream &&
                        p->backend.flush(p->backend_user, open_file->stream) != 0) {
                        open_file->error = 1u;
                        result->r0 = (uint32_t)-1;
                    }
                }
            }
        } else {
            f = resolve_file(p, a);
            if (!f) return RE_IMPORT_E_HANDLER;
            if (p->backend.flush && p->backend.flush(p->backend_user, f->stream) != 0) {
                f->error = 1u;
                result->r0 = (uint32_t)-1;
            }
        }
        break;
    default:
        return RE_IMPORT_E_HANDLER;
    }
    return RE_IMPORT_OK;
}

static int find_symbol(const REGUESTELF32LINKSYMBOL *symbols, size_t count,
                       const char *name, size_t *found) {
    size_t match = count;
    for (size_t i = 0; i < count; ++i) {
        const REGUESTELF32LINKSYMBOL *s = &symbols[i];
        if (!memchr(s->name, '\0', sizeof(s->name))) return 0;
        if (strcmp(s->name, name) || s->section_index != 0u ||
            (s->binding != RE_GUEST_STB_GLOBAL && s->binding != RE_GUEST_STB_WEAK) ||
            (s->visibility != RE_GUEST_STV_DEFAULT && s->visibility != RE_GUEST_STV_PROTECTED) ||
            (s->type != 0u && s->type != 2u)) continue;
        if (match != count) return 0;
        match = i;
    }
    if (match == count) return 0;
    *found = match;
    return 1;
}

REGUESTLIBCStdioStatus re_guest_libc_stdio_provider_init(
    REGUESTLIBCStdioProvider *p, const REGUESTLIBCStdioBackend *backend,
    void *user) {
    if (!p || !backend || !backend->open || !backend->read ||
        !backend->write || !backend->seek || !backend->tell || !backend->close)
        return RE_GUEST_LIBC_STDIO_E_ARGUMENT;
    memset(p, 0, sizeof(*p));
    p->backend = *backend;
    p->backend_user = user;
    p->next_generation = 1u;
    for (size_t i = 0; i < RE_GUEST_LIBC_STDIO_SYMBOL_COUNT; ++i) {
        p->contexts[i].kind = (uint32_t)i;
        p->contexts[i].provider = p;
    }
    p->initialized = 1u;
    return RE_GUEST_LIBC_STDIO_OK;
}

REGUESTLIBCStdioStatus re_guest_libc_stdio_provider_bind_elf_symbols(
    REGUESTLIBCStdioProvider *p, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index) {
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[RE_GUEST_LIBC_STDIO_SYMBOL_COUNT];
    size_t indices[RE_GUEST_LIBC_STDIO_SYMBOL_COUNT];
    uint32_t pointers[RE_GUEST_LIBC_STDIO_SYMBOL_COUNT];
    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!p || !p->initialized || !imports || !symbols || !exports ||
        !export_count || export_capacity < RE_GUEST_LIBC_STDIO_SYMBOL_COUNT)
        return RE_GUEST_LIBC_STDIO_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - RE_GUEST_LIBC_STDIO_SYMBOL_COUNT)
        return RE_GUEST_LIBC_STDIO_E_CAPACITY;
    for (size_t i = 0; i < RE_GUEST_LIBC_STDIO_SYMBOL_COUNT; ++i) {
        if (!find_symbol(symbols, symbol_count, k_names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = symbol_count;
            return RE_GUEST_LIBC_STDIO_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j)
            if (indices[i] == indices[j]) return RE_GUEST_LIBC_STDIO_E_SYMBOL;
    }
    backup = *imports;
    for (size_t i = 0; i < RE_GUEST_LIBC_STDIO_SYMBOL_COUNT; ++i) {
        if (re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
                stdio_handler, &p->contexts[i], &pointers[i]) != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_LIBC_STDIO_E_IMPORT;
        }
        pending[i] = (REGUESTSYMBOL){
            k_names[i], pointers[i], RE_GUEST_STB_GLOBAL,
            RE_GUEST_STV_DEFAULT, 1u, NULL, "libc.so"
        };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = RE_GUEST_LIBC_STDIO_SYMBOL_COUNT;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_LIBC_STDIO_OK;
}

void re_guest_libc_stdio_provider_close_all(REGUESTLIBCStdioProvider *p) {
    if (!p || !p->initialized) return;
    for (size_t i = 0; i < RE_GUEST_LIBC_STDIO_MAX_FILES; ++i) {
        REGUESTLIBCStdioFile *f = &p->files[i];
        if (f->active && f->stream) {
            (void)p->backend.close(p->backend_user, f->stream);
            f->stream = NULL;
            f->active = 0u;
        }
    }
}
