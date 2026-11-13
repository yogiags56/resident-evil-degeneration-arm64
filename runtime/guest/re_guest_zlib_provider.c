#include "re_guest_zlib_provider.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "re_guest_memory_bus.h"

enum {
    ZS_NEXT_IN = 0,
    ZS_AVAIL_IN = 4,
    ZS_TOTAL_IN = 8,
    ZS_NEXT_OUT = 12,
    ZS_AVAIL_OUT = 16,
    ZS_TOTAL_OUT = 20,
    ZS_MSG = 24,
    ZS_STATE = 28,
    ZS_DATA_TYPE = 44,
    ZS_ADLER = 48,
    ZS_RESERVED = 52
};

typedef struct {
    uint32_t words[14];
} GuestZStream32;

_Static_assert(sizeof(GuestZStream32) == RE_GUEST_ZLIB_STREAM32_SIZE,
               "ARM32 z_stream ABI must remain exactly 56 bytes");

static int read_guest_string(const REGUESTMEMBUS *bus, uint32_t address,
                             char *out, size_t capacity) {
    if (!bus || !bus->memory || !address || !out || capacity < 2u) return 0;
    for (size_t i = 0; i < capacity; ++i) {
        uint8_t byte = 0;
        uint64_t at = (uint64_t)address + i;
        if (at > UINT32_MAX ||
            re_guest_memory_read8(bus, (uint32_t)at, &byte) != RE_GUEST_MEM_OK)
            return 0;
        if (!byte) {
            out[i] = '\0';
            return 1;
        }
        if (byte < 0x20u || byte > 0x7eu) return 0;
        out[i] = (char)byte;
    }
    out[capacity - 1u] = '\0';
    return 0;
}

static int guest_stream_access(const REGUESTMEMBUS *bus, uint32_t address,
                               void **host_stream) {
    if (host_stream) *host_stream = NULL;
    if (!bus || !bus->memory || !host_stream || !address || (address & 3u))
        return 0;
    return re_guest_memory_resolve(bus->memory, address,
        RE_GUEST_ZLIB_STREAM32_SIZE, RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE,
        host_stream) == RE_GUEST_MEM_OK;
}

static int read_word(const REGUESTMEMBUS *bus, uint32_t base,
                     uint32_t offset, uint32_t *value) {
    uint64_t address = (uint64_t)base + offset;
    if (address > UINT32_MAX) return 0;
    return re_guest_memory_read32(bus, (uint32_t)address, value) ==
           RE_GUEST_MEM_OK;
}

static int write_word(REGUESTMEMBUS *bus, uint32_t base,
                      uint32_t offset, uint32_t value) {
    uint64_t address = (uint64_t)base + offset;
    if (address > UINT32_MAX) return 0;
    return re_guest_memory_write32(bus, (uint32_t)address, value) ==
           RE_GUEST_MEM_OK;
}

static REGUESTZLIBStreamSlot *find_stream(REGUESTZLIBProvider *provider,
                                          uint32_t guest_stream,
                                          uint32_t state_token) {
    if (!provider) return NULL;
    for (size_t i = 0; i < RE_GUEST_ZLIB_MAX_STREAMS; ++i) {
        REGUESTZLIBStreamSlot *slot = &provider->streams[i];
        if (slot->active && slot->guest_stream == guest_stream &&
            slot->guest_state_token == state_token)
            return slot;
    }
    return NULL;
}

static int find_free_slot(REGUESTZLIBProvider *provider, size_t *index) {
    for (size_t i = 0; i < RE_GUEST_ZLIB_MAX_STREAMS; ++i) {
        if (!provider->streams[i].active) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

static int initialize_stream(REGUESTZLIBProvider *provider,
                             const REGUESTMEMBUS *bus,
                             uint32_t guest_stream, int window_bits,
                             uint32_t version_address,
                             uint32_t guest_stream_size) {
    REGUESTMEMBUS writable_bus;
    REGUESTZLIBStreamSlot *slot;
    z_stream *native;
    char guest_version[16];
    void *mapped_stream = NULL;
    size_t slot_index = 0u;
    uint32_t existing_state = 0u;
    int status;

    if (!provider || !bus || !guest_stream ||
        guest_stream_size != RE_GUEST_ZLIB_STREAM32_SIZE ||
        !guest_stream_access(bus, guest_stream, &mapped_stream) ||
        !read_word(bus, guest_stream, ZS_STATE, &existing_state))
        return Z_VERSION_ERROR;
    if (existing_state != 0u) return Z_STREAM_ERROR;
    if (!read_guest_string(bus, version_address, guest_version,
                           sizeof(guest_version)) ||
        guest_version[0] != ZLIB_VERSION[0])
        return Z_VERSION_ERROR;
    if (!find_free_slot(provider, &slot_index)) return Z_MEM_ERROR;

    native = (z_stream *)calloc(1u, sizeof(*native));
    if (!native) return Z_MEM_ERROR;
    native->zalloc = Z_NULL;
    native->zfree = Z_NULL;
    native->opaque = Z_NULL;
    status = inflateInit2_(native, window_bits, ZLIB_VERSION,
                           (int)sizeof(*native));
    if (status != Z_OK) {
        free(native);
        return status;
    }

    slot = &provider->streams[slot_index];
    slot->guest_stream = guest_stream;
    slot->guest_state_token = provider->token_base + (uint32_t)(slot_index * 4u);
    slot->native_state = native;
    slot->busy = 0u;
    slot->active = 1u;

    writable_bus = *bus;
    if (!write_word(&writable_bus, guest_stream, ZS_TOTAL_IN, 0u) ||
        !write_word(&writable_bus, guest_stream, ZS_TOTAL_OUT, 0u) ||
        !write_word(&writable_bus, guest_stream, ZS_MSG, 0u) ||
        !write_word(&writable_bus, guest_stream, ZS_STATE,
                    slot->guest_state_token) ||
        !write_word(&writable_bus, guest_stream, ZS_DATA_TYPE,
                    (uint32_t)native->data_type) ||
        !write_word(&writable_bus, guest_stream, ZS_ADLER,
                    (uint32_t)native->adler) ||
        !write_word(&writable_bus, guest_stream, ZS_RESERVED, 0u)) {
        (void)inflateEnd(native);
        free(native);
        memset(slot, 0, sizeof(*slot));
        return Z_STREAM_ERROR;
    }
    return Z_OK;
}

static REGUESTIMPORTStatus init_handler_common(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result, int has_window_bits) {
    REGUESTZLIBProvider *provider = (REGUESTZLIBProvider *)user;
    uint32_t stream_size;
    int window_bits = MAX_WBITS;
    if (!frame || !bus || !provider || !result) return RE_IMPORT_E_HANDLER;
    if (has_window_bits) {
        window_bits = (int32_t)frame->r[1];
        stream_size = frame->r[3];
    } else {
        stream_size = frame->r[2];
    }
    result->r0 = (uint32_t)initialize_stream(provider, bus, frame->r[0],
        window_bits, has_window_bits ? frame->r[2] : frame->r[1], stream_size);
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus inflate_init_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    return init_handler_common(frame, bus, user, result, 0);
}

static REGUESTIMPORTStatus inflate_init2_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    return init_handler_common(frame, bus, user, result, 1);
}

static REGUESTIMPORTStatus inflate_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTZLIBProvider *provider = (REGUESTZLIBProvider *)user;
    REGUESTZLIBStreamSlot *slot;
    REGUESTMEMBUS writable_bus;
    z_stream *native;
    uint32_t next_in, avail_in, total_in, next_out, avail_out, total_out;
    uint32_t state_token, flush;
    uint32_t in_count, out_count, consumed, produced;
    void *guest_input = NULL, *guest_output = NULL, *mapped_stream = NULL;
    uint8_t *input_copy = NULL, *output_copy = NULL;
    int status;

    if (!frame || !bus || !provider || !result) return RE_IMPORT_E_HANDLER;
    if (!guest_stream_access(bus, frame->r[0], &mapped_stream) ||
        !read_word(bus, frame->r[0], ZS_STATE, &state_token)) {
        result->r0 = (uint32_t)Z_STREAM_ERROR;
        result->r1 = 0u;
        return RE_IMPORT_OK;
    }
    slot = find_stream(provider, frame->r[0], state_token);
    if (!slot || slot->busy || !slot->native_state) {
        result->r0 = (uint32_t)Z_STREAM_ERROR;
        result->r1 = 0u;
        return RE_IMPORT_OK;
    }
    if (!read_word(bus, frame->r[0], ZS_NEXT_IN, &next_in) ||
        !read_word(bus, frame->r[0], ZS_AVAIL_IN, &avail_in) ||
        !read_word(bus, frame->r[0], ZS_TOTAL_IN, &total_in) ||
        !read_word(bus, frame->r[0], ZS_NEXT_OUT, &next_out) ||
        !read_word(bus, frame->r[0], ZS_AVAIL_OUT, &avail_out) ||
        !read_word(bus, frame->r[0], ZS_TOTAL_OUT, &total_out)) {
        result->r0 = (uint32_t)Z_STREAM_ERROR;
        result->r1 = 0u;
        return RE_IMPORT_OK;
    }
    flush = frame->r[1];
    if (flush > 6u) {
        result->r0 = (uint32_t)Z_STREAM_ERROR;
        result->r1 = 0u;
        return RE_IMPORT_OK;
    }
    in_count = avail_in > RE_GUEST_ZLIB_IO_CHUNK_CAP
        ? RE_GUEST_ZLIB_IO_CHUNK_CAP : avail_in;
    out_count = avail_out > RE_GUEST_ZLIB_IO_CHUNK_CAP
        ? RE_GUEST_ZLIB_IO_CHUNK_CAP : avail_out;
    if (in_count) {
        uint64_t end = (uint64_t)next_in + in_count;
        if (!next_in || end > (uint64_t)UINT32_MAX ||
            re_guest_memory_resolve(bus->memory, next_in, in_count,
                RE_GUEST_MEM_READ, &guest_input) != RE_GUEST_MEM_OK) {
            result->r0 = (uint32_t)Z_STREAM_ERROR;
            result->r1 = 0u;
            return RE_IMPORT_OK;
        }
        input_copy = (uint8_t *)malloc(in_count);
        if (!input_copy) {
            result->r0 = (uint32_t)Z_MEM_ERROR;
            result->r1 = 0u;
            return RE_IMPORT_OK;
        }
        memcpy(input_copy, guest_input, in_count);
    }
    if (out_count) {
        uint64_t end = (uint64_t)next_out + out_count;
        if (!next_out || end > (uint64_t)UINT32_MAX ||
            re_guest_memory_resolve(bus->memory, next_out, out_count,
                RE_GUEST_MEM_WRITE, &guest_output) != RE_GUEST_MEM_OK) {
            free(input_copy);
            result->r0 = (uint32_t)Z_STREAM_ERROR;
            result->r1 = 0u;
            return RE_IMPORT_OK;
        }
        output_copy = (uint8_t *)malloc(out_count);
        if (!output_copy) {
            free(input_copy);
            result->r0 = (uint32_t)Z_MEM_ERROR;
            result->r1 = 0u;
            return RE_IMPORT_OK;
        }
    }

    slot->busy = 1u;
    native = (z_stream *)slot->native_state;
    native->next_in = input_copy;
    native->avail_in = (uInt)in_count;
    native->next_out = output_copy;
    native->avail_out = (uInt)out_count;
    status = inflate(native, (int)flush);
    consumed = in_count - native->avail_in;
    produced = out_count - native->avail_out;

    if ((uint64_t)next_in + consumed > UINT32_MAX ||
        (uint64_t)next_out + produced > UINT32_MAX) {
        status = Z_STREAM_ERROR;
    } else {
        writable_bus = *bus;
        if (produced) memcpy(guest_output, output_copy, produced);
        if (!write_word(&writable_bus, frame->r[0], ZS_NEXT_IN,
                        next_in + consumed) ||
            !write_word(&writable_bus, frame->r[0], ZS_AVAIL_IN,
                        avail_in - consumed) ||
            !write_word(&writable_bus, frame->r[0], ZS_TOTAL_IN,
                        total_in + consumed) ||
            !write_word(&writable_bus, frame->r[0], ZS_NEXT_OUT,
                        next_out + produced) ||
            !write_word(&writable_bus, frame->r[0], ZS_AVAIL_OUT,
                        avail_out - produced) ||
            !write_word(&writable_bus, frame->r[0], ZS_TOTAL_OUT,
                        total_out + produced) ||
            !write_word(&writable_bus, frame->r[0], ZS_MSG, 0u) ||
            !write_word(&writable_bus, frame->r[0], ZS_DATA_TYPE,
                        (uint32_t)native->data_type) ||
            !write_word(&writable_bus, frame->r[0], ZS_ADLER,
                        (uint32_t)native->adler)) {
            status = Z_STREAM_ERROR;
        }
    }
    slot->busy = 0u;
    free(input_copy);
    free(output_copy);
    result->r0 = (uint32_t)status;
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus inflate_end_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTZLIBProvider *provider = (REGUESTZLIBProvider *)user;
    REGUESTZLIBStreamSlot *slot;
    REGUESTMEMBUS writable_bus;
    void *mapped_stream = NULL;
    uint32_t token = 0u;
    int status;
    if (!frame || !bus || !provider || !result) return RE_IMPORT_E_HANDLER;
    if (!guest_stream_access(bus, frame->r[0], &mapped_stream) ||
        !read_word(bus, frame->r[0], ZS_STATE, &token)) {
        result->r0 = (uint32_t)Z_STREAM_ERROR;
        result->r1 = 0u;
        return RE_IMPORT_OK;
    }
    slot = find_stream(provider, frame->r[0], token);
    if (!slot || slot->busy || !slot->native_state) {
        result->r0 = (uint32_t)Z_STREAM_ERROR;
        result->r1 = 0u;
        return RE_IMPORT_OK;
    }
    slot->busy = 1u;
    status = inflateEnd((z_stream *)slot->native_state);
    if (status == Z_OK) {
        writable_bus = *bus;
        (void)write_word(&writable_bus, frame->r[0], ZS_STATE, 0u);
        (void)write_word(&writable_bus, frame->r[0], ZS_MSG, 0u);
        free(slot->native_state);
        memset(slot, 0, sizeof(*slot));
    } else {
        slot->busy = 0u;
    }
    result->r0 = (uint32_t)status;
    result->r1 = 0u;
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

REGUESTZLIBStatus re_guest_zlib_provider_init(
    REGUESTZLIBProvider *provider, uint32_t token_base) {
    if (!provider || !token_base || (token_base & 3u) ||
        token_base > UINT32_MAX -
            (uint32_t)((RE_GUEST_ZLIB_MAX_STREAMS - 1u) * 4u))
        return RE_GUEST_ZLIB_E_ARGUMENT;
    memset(provider, 0, sizeof(*provider));
    provider->token_base = token_base;
    return RE_GUEST_ZLIB_OK;
}

REGUESTZLIBStatus re_guest_zlib_provider_bind_elf_symbols(
    REGUESTZLIBProvider *provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols,
    size_t symbol_count,
    REGUESTSYMBOL *exports,
    size_t export_capacity,
    size_t *export_count,
    size_t *failed_symbol_index) {
    static const char *const names[4] = {
        "inflate", "inflateEnd", "inflateInit2_", "inflateInit_"
    };
    REGUESTIMPORTMappedHandler handlers[4] = {
        inflate_handler, inflate_end_handler,
        inflate_init2_handler, inflate_init_handler
    };
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[4];
    size_t indices[4] = {symbol_count, symbol_count, symbol_count, symbol_count};
    uint32_t pointers[4] = {0u, 0u, 0u, 0u};
    REGUESTIMPORTStatus status;

    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !imports || !symbols || !exports || !export_count ||
        export_capacity < 4u)
        return RE_GUEST_ZLIB_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - 4u)
        return RE_GUEST_ZLIB_E_CAPACITY;
    for (size_t i = 0; i < 4u; ++i) {
        if (!find_undefined_symbol(symbols, symbol_count, names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = indices[i];
            return RE_GUEST_ZLIB_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j) {
            if (indices[i] == indices[j]) return RE_GUEST_ZLIB_E_SYMBOL;
        }
    }

    backup = *imports;
    for (size_t i = 0; i < 4u; ++i) {
        status = re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
            handlers[i], provider, &pointers[i]);
        if (status != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_ZLIB_E_IMPORT;
        }
        pending[i] = (REGUESTSYMBOL){
            names[i], pointers[i], RE_GUEST_STB_GLOBAL,
            RE_GUEST_STV_DEFAULT, 1u, NULL, "libz.so"
        };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = 4u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_ZLIB_OK;
}

void re_guest_zlib_provider_shutdown(REGUESTZLIBProvider *provider) {
    if (!provider) return;
    for (size_t i = 0; i < RE_GUEST_ZLIB_MAX_STREAMS; ++i) {
        REGUESTZLIBStreamSlot *slot = &provider->streams[i];
        if (slot->active && slot->native_state) {
            (void)inflateEnd((z_stream *)slot->native_state);
            free(slot->native_state);
        }
        memset(slot, 0, sizeof(*slot));
    }
}
