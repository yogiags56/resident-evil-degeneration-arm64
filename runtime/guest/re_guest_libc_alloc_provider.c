#include "re_guest_libc_alloc_provider.h"

#include <string.h>

static REGUESTIMPORTStatus malloc_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLIBCAllocProvider *provider = (REGUESTLIBCAllocProvider *)user;
    if (!frame || !bus || !provider || !provider->heap || !result)
        return RE_IMPORT_E_HANDLER;
    result->r0 = re_guest_heap_alloc(provider->heap, bus, frame->r[0],
                                     RE_GUEST_HEAP_DEFAULT_ALIGNMENT);
    result->r1 = result->r2 = result->r3 = 0u;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus free_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLIBCAllocProvider *provider = (REGUESTLIBCAllocProvider *)user;
    (void)bus;
    if (!frame || !provider || !provider->heap || !result)
        return RE_IMPORT_E_HANDLER;
    re_guest_heap_free(provider->heap, frame->r[0]);
    result->r0 = result->r1 = result->r2 = result->r3 = 0u;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus realloc_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLIBCAllocProvider *provider = (REGUESTLIBCAllocProvider *)user;
    if (!frame || !bus || !provider || !provider->heap || !result)
        return RE_IMPORT_E_HANDLER;
    result->r0 = re_guest_heap_realloc(provider->heap, bus, frame->r[0],
                                       frame->r[1]);
    result->r1 = result->r2 = result->r3 = 0u;
    return RE_IMPORT_OK;
}

static REGUESTIMPORTStatus valloc_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTLIBCAllocProvider *provider = (REGUESTLIBCAllocProvider *)user;
    if (!frame || !bus || !provider || !provider->heap || !result)
        return RE_IMPORT_E_HANDLER;
    result->r0 = re_guest_heap_alloc(provider->heap, bus, frame->r[0],
                                     RE_GUEST_HEAP_PAGE_ALIGNMENT);
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

REGUESTLIBCAllocStatus re_guest_libc_alloc_provider_init(
    REGUESTLIBCAllocProvider *provider, REGUESTHEAP *heap) {
    if (!provider || !heap || !heap->heap_base || !heap->heap_size)
        return RE_GUEST_LIBC_ALLOC_E_ARGUMENT;
    provider->heap = heap;
    return RE_GUEST_LIBC_ALLOC_OK;
}

REGUESTLIBCAllocStatus re_guest_libc_alloc_provider_bind_elf_symbols(
    REGUESTLIBCAllocProvider *provider, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index) {
    static const char *const names[RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT] = {
        "malloc", "free", "realloc", "valloc"
    };
    static const REGUESTIMPORTMappedHandler handlers[RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT] = {
        malloc_handler, free_handler, realloc_handler, valloc_handler
    };
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT];
    size_t indices[RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT];
    uint32_t pointers[RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT];

    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !provider->heap || !imports || !symbols || !exports ||
        !export_count || export_capacity < RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT)
        return RE_GUEST_LIBC_ALLOC_E_ARGUMENT;
    if (symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT)
        return RE_GUEST_LIBC_ALLOC_E_CAPACITY;

    for (size_t i = 0; i < RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT; ++i) {
        if (!find_undefined_symbol(symbols, symbol_count, names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = indices[i];
            return RE_GUEST_LIBC_ALLOC_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j)
            if (indices[i] == indices[j]) return RE_GUEST_LIBC_ALLOC_E_SYMBOL;
    }

    backup = *imports;
    for (size_t i = 0; i < RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT; ++i) {
        if (re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
                handlers[i], provider, &pointers[i]) != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_LIBC_ALLOC_E_IMPORT;
        }
        pending[i] = (REGUESTSYMBOL){
            names[i], pointers[i], RE_GUEST_STB_GLOBAL,
            RE_GUEST_STV_DEFAULT, 1u, NULL, "libc.so"
        };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_LIBC_ALLOC_OK;
}
