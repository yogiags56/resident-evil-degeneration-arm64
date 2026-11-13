#include "re_guest_libc_format_provider.h"

#include <string.h>

#include "re_guest_format.h"

enum { FORMAT_SNPRINTF = 0u, FORMAT_SPRINTF = 1u };

static const char *const k_names[RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT] = {
    "snprintf", "sprintf"
};

static int read_guest_format(const REGUESTMEMBUS *bus, uint32_t address,
                             char *out, size_t capacity) {
    if (!bus || !bus->memory || !address || !out || !capacity) return 0;
    for (size_t i = 0; i < capacity; ++i) {
        uint64_t at = (uint64_t)address + i;
        uint8_t byte;
        if (at > UINT32_MAX ||
            re_guest_memory_read8(bus, (uint32_t)at, &byte) != RE_GUEST_MEM_OK)
            return 0;
        if (!byte) {
            out[i] = '\0';
            return 1;
        }
        out[i] = (char)byte;
    }
    return 0;
}

static REGUESTIMPORTStatus format_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    const REGUESTLIBCFormatContext *context =
        (const REGUESTLIBCFormatContext *)user;
    REGUESTLIBCFormatProvider *provider;
    char format[RE_GUEST_FORMAT_STRING_CAP];
    char output[RE_GUEST_FORMAT_OUTPUT_CAP];
    uint32_t destination, size, format_address, first_word_index;
    size_t output_length = 0u, copy_length;
    void *host_destination = NULL;

    if (!frame || !bus || !bus->memory || !context || !result)
        return RE_IMPORT_E_HANDLER;
    provider = context->provider;
    if (!provider || !provider->initialized) return RE_IMPORT_E_HANDLER;

    if (context->kind == FORMAT_SNPRINTF) {
        destination = frame->r[0];
        size = frame->r[1];
        format_address = frame->r[2];
        first_word_index = 3u;
    } else if (context->kind == FORMAT_SPRINTF) {
        destination = frame->r[0];
        size = UINT32_MAX;
        format_address = frame->r[1];
        first_word_index = 2u;
    } else {
        return RE_IMPORT_E_HANDLER;
    }

    if (!read_guest_format(bus, format_address, format, sizeof(format)) ||
        !re_guest_format_guest_printf(frame, bus, first_word_index, format,
            output, sizeof(output), &output_length))
        return RE_IMPORT_E_HANDLER;

    if (context->kind == FORMAT_SPRINTF) {
        if (output_length >= UINT32_MAX) return RE_IMPORT_E_HANDLER;
        size = (uint32_t)output_length + 1u;
    }
    if (size) {
        if (!destination) return RE_IMPORT_E_HANDLER;
        copy_length = output_length;
        if (copy_length >= size) copy_length = (size_t)size - 1u;
        if (re_guest_memory_resolve(bus->memory, destination,
                copy_length + 1u, RE_GUEST_MEM_WRITE,
                &host_destination) != RE_GUEST_MEM_OK)
            return RE_IMPORT_E_HANDLER;
        if (copy_length) memcpy(host_destination, output, copy_length);
        ((char *)host_destination)[copy_length] = '\0';
    }

    result->r0 = (uint32_t)output_length;
    result->r1 = result->r2 = result->r3 = 0u;
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

REGUESTLIBCFormatStatus re_guest_libc_format_provider_init(
    REGUESTLIBCFormatProvider *provider) {
    if (!provider) return RE_GUEST_LIBC_FORMAT_E_ARGUMENT;
    memset(provider, 0, sizeof(*provider));
    for (size_t i = 0; i < RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT; ++i)
        provider->contexts[i].provider = provider;
    provider->initialized = 1u;
    return RE_GUEST_LIBC_FORMAT_OK;
}

REGUESTLIBCFormatStatus re_guest_libc_format_provider_bind_elf_symbols(
    REGUESTLIBCFormatProvider *provider, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index) {
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT];
    size_t indices[RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT];
    uint32_t pointers[RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT];

    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !provider->initialized || !imports || !symbols ||
        !exports || !export_count ||
        export_capacity < RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT)
        return RE_GUEST_LIBC_FORMAT_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT)
        return RE_GUEST_LIBC_FORMAT_E_CAPACITY;

    for (size_t i = 0; i < RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT; ++i) {
        if (!find_undefined_symbol(symbols, symbol_count, k_names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = indices[i];
            return RE_GUEST_LIBC_FORMAT_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j)
            if (indices[i] == indices[j]) return RE_GUEST_LIBC_FORMAT_E_SYMBOL;
    }

    backup = *imports;
    for (size_t i = 0; i < RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT; ++i) {
        provider->contexts[i].kind = (uint32_t)i;
        provider->contexts[i].provider = provider;
        if (re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
                format_handler, &provider->contexts[i], &pointers[i]) != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_LIBC_FORMAT_E_IMPORT;
        }
        pending[i] = (REGUESTSYMBOL){
            k_names[i], pointers[i], RE_GUEST_STB_GLOBAL,
            RE_GUEST_STV_DEFAULT, 1u, NULL, "libc.so"
        };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_LIBC_FORMAT_OK;
}
