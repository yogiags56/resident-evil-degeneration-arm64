#include "re_guest_cxx_heap_provider.h"

#include <string.h>

static REGUESTIMPORTStatus new_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTCXXHeapProvider *provider = (REGUESTCXXHeapProvider *)user;
    if (!frame || !bus || !provider || !result) return RE_IMPORT_E_HANDLER;
    result->r0 = re_guest_heap_alloc(&provider->heap, bus, frame->r[0],
                                     RE_GUEST_CXX_HEAP_ALIGNMENT);
    result->r1 = result->r2 = result->r3 = 0u;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus delete_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTCXXHeapProvider *provider = (REGUESTCXXHeapProvider *)user;
    (void)bus;
    if (!frame || !provider || !result) return RE_IMPORT_E_HANDLER;
    re_guest_heap_free(&provider->heap, frame->r[0]);
    result->r0 = result->r1 = result->r2 = result->r3 = 0u;
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

REGUESTCXXHeapStatus re_guest_cxx_heap_provider_init(
    REGUESTCXXHeapProvider *provider, uint32_t heap_base, uint32_t heap_size) {
    if (!provider) return RE_GUEST_CXX_HEAP_E_ARGUMENT;
    memset(provider, 0, sizeof(*provider));
    return re_guest_heap_init(&provider->heap, heap_base, heap_size) ==
        RE_GUEST_HEAP_OK ? RE_GUEST_CXX_HEAP_OK :
        RE_GUEST_CXX_HEAP_E_ARGUMENT;
}

REGUESTCXXHeapStatus re_guest_cxx_heap_provider_bind_elf_symbols(
    REGUESTCXXHeapProvider *provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols,
    size_t symbol_count,
    REGUESTSYMBOL *exports,
    size_t export_capacity,
    size_t *export_count,
    size_t *failed_symbol_index) {
    static const char *const names[4] = {
        "_Znwj", "_Znaj", "_ZdlPv", "_ZdaPv"
    };
    REGUESTIMPORTMappedHandler handlers[4] = {
        new_handler, new_handler, delete_handler, delete_handler
    };
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[4];
    size_t indices[4] = {symbol_count, symbol_count, symbol_count, symbol_count};
    uint32_t pointers[4] = {0u, 0u, 0u, 0u};

    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !provider->heap.heap_base || !imports || !symbols ||
        !exports || !export_count || export_capacity < 4u)
        return RE_GUEST_CXX_HEAP_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - 4u)
        return RE_GUEST_CXX_HEAP_E_CAPACITY;
    for (size_t i = 0; i < 4u; ++i) {
        if (!find_undefined_symbol(symbols, symbol_count, names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = indices[i];
            return RE_GUEST_CXX_HEAP_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j) {
            if (indices[i] == indices[j]) return RE_GUEST_CXX_HEAP_E_SYMBOL;
        }
    }

    backup = *imports;
    for (size_t i = 0; i < 4u; ++i) {
        if (re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
                handlers[i], provider, &pointers[i]) != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_CXX_HEAP_E_IMPORT;
        }
        pending[i] = (REGUESTSYMBOL){
            names[i], pointers[i], RE_GUEST_STB_GLOBAL,
            RE_GUEST_STV_DEFAULT, 1u, NULL, "libstdc++.so"
        };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = 4u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_CXX_HEAP_OK;
}

void re_guest_cxx_heap_provider_shutdown(REGUESTCXXHeapProvider *provider) {
    if (provider) re_guest_heap_shutdown(&provider->heap);
}
