#include "re_guest_log_provider.h"

#include <string.h>

#include "re_guest_format.h"
#include "re_guest_memory_bus.h"

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


static REGUESTIMPORTStatus log_write_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLOGProvider *provider = (REGUESTLOGProvider *)user;
    char tag[RE_GUEST_LOG_TAG_CAP];
    char message[RE_GUEST_LOG_MESSAGE_CAP];
    if (!frame || !bus || !provider || !provider->write_callback || !result)
        return RE_IMPORT_E_HANDLER;
    if (!read_guest_string(bus, frame->r[1], tag, sizeof(tag)) ||
        !read_guest_string(bus, frame->r[2], message, sizeof(message))) {
        result->r0 = (uint32_t)-22;
        result->r1 = 0;
        return RE_IMPORT_OK;
    }
    result->r0 = (uint32_t)provider->write_callback(
        (int32_t)frame->r[0], tag, message, provider->user);
    result->r1 = 0;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus log_print_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLOGProvider *provider = (REGUESTLOGProvider *)user;
    char tag[RE_GUEST_LOG_TAG_CAP];
    char format[RE_GUEST_LOG_FORMAT_CAP];
    char message[RE_GUEST_LOG_MESSAGE_CAP];
    if (!frame || !bus || !provider || !provider->write_callback || !result)
        return RE_IMPORT_E_HANDLER;
    if (!read_guest_string(bus, frame->r[1], tag, sizeof(tag)) ||
        !read_guest_string(bus, frame->r[2], format, sizeof(format)) ||
        !re_guest_format_guest_printf(frame, bus, 3u, format, message, sizeof(message), NULL)) {
        result->r0 = (uint32_t)-22;
        result->r1 = 0;
        return RE_IMPORT_OK;
    }
    result->r0 = (uint32_t)provider->write_callback(
        (int32_t)frame->r[0], tag, message, provider->user);
    result->r1 = 0;
    return RE_IMPORT_OK;
}

static int find_undefined_symbol(
    const REGUESTELF32LINKSYMBOL *symbols, size_t count,
    const char *name, size_t *found) {
    size_t match = count;
    for (size_t i = 0; i < count; ++i) {
        const REGUESTELF32LINKSYMBOL *s = &symbols[i];
        if (!memchr(s->name, '\0', sizeof(s->name))) return 0;
        if (strcmp(s->name, name) != 0 || s->section_index != 0u ||
            (s->binding != RE_GUEST_STB_GLOBAL &&
             s->binding != RE_GUEST_STB_WEAK) ||
            (s->visibility != RE_GUEST_STV_DEFAULT &&
             s->visibility != RE_GUEST_STV_PROTECTED) ||
            (s->type != 0u && s->type != 2u))
            continue;
        if (match != count) return 0;
        match = i;
    }
    if (match == count) return 0;
    *found = match;
    return 1;
}

REGUESTLOGStatus re_guest_log_provider_bind_elf_symbols(
    REGUESTLOGProvider *provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols,
    size_t symbol_count,
    REGUESTSYMBOL *exports,
    size_t export_capacity,
    size_t *export_count,
    size_t *failed_symbol_index) {
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[2];
    size_t print_index = symbol_count;
    size_t write_index = symbol_count;
    uint32_t print_pointer = 0, write_pointer = 0;
    REGUESTIMPORTStatus status;

    if (export_count) *export_count = 0;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !provider->write_callback || !imports || !symbols ||
        !exports || !export_count || export_capacity < 2u)
        return RE_GUEST_LOG_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS)
        return RE_GUEST_LOG_E_CAPACITY;
    if (!find_undefined_symbol(symbols, symbol_count,
                               "__android_log_print", &print_index)) {
        if (failed_symbol_index) *failed_symbol_index = print_index;
        return RE_GUEST_LOG_E_SYMBOL;
    }
    if (!find_undefined_symbol(symbols, symbol_count,
                               "__android_log_write", &write_index)) {
        if (failed_symbol_index) *failed_symbol_index = write_index;
        return RE_GUEST_LOG_E_SYMBOL;
    }
    if (print_index == write_index ||
        imports->count > RE_IMPORT_MAX_BINDINGS - 2u)
        return RE_GUEST_LOG_E_CAPACITY;

    backup = *imports;
    status = re_guest_import_bind_mapped(imports, (uint32_t)print_index,
        log_print_handler, provider, &print_pointer);
    if (status == RE_IMPORT_OK) {
        status = re_guest_import_bind_mapped(imports, (uint32_t)write_index,
            log_write_handler, provider, &write_pointer);
    }
    if (status != RE_IMPORT_OK) {
        *imports = backup;
        return RE_GUEST_LOG_E_IMPORT;
    }

    pending[0] = (REGUESTSYMBOL){
        "__android_log_print", print_pointer,
        RE_GUEST_STB_GLOBAL, RE_GUEST_STV_DEFAULT, 1u, NULL, "liblog.so"
    };
    pending[1] = (REGUESTSYMBOL){
        "__android_log_write", write_pointer,
        RE_GUEST_STB_GLOBAL, RE_GUEST_STV_DEFAULT, 1u, NULL, "liblog.so"
    };
    memcpy(exports, pending, sizeof(pending));
    *export_count = 2u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_LOG_OK;
}
