#include "re_guest_dl_provider_catalog.h"

#include <string.h>

static int valid_soname(const char *name) {
    if (!name || !name[0] || strlen(name) >= RE_GUEST_DL_PATH_CAP ||
        strcmp(name, ".") == 0 || strcmp(name, "..") == 0 ||
        strchr(name, '/') || strchr(name, '\\'))
        return 0;
    for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
        if (*p < 0x21u || *p > 0x7eu) return 0;
    }
    return 1;
}

static int valid_export(const REGUESTSYMBOL *symbol, const char *soname) {
    if (!symbol || !symbol->name || !symbol->name[0] ||
        strlen(symbol->name) >= RE_GUEST_DL_SYMBOL_CAP ||
        !symbol->defined ||
        (symbol->binding != RE_GUEST_STB_GLOBAL &&
         symbol->binding != RE_GUEST_STB_WEAK) ||
        (symbol->visibility != RE_GUEST_STV_DEFAULT &&
         symbol->visibility != RE_GUEST_STV_PROTECTED))
        return 0;
    if (symbol->version_library &&
        strcmp(symbol->version_library, soname) != 0)
        return 0;
    return 1;
}

REGUESTDLProviderStatus re_guest_dl_provider_catalog_init(
    REGUESTDLProviderCatalog *catalog,
    const REGUESTDLProviderSource *sources,
    size_t source_count,
    size_t *failed_source) {
    REGUESTDLProviderCatalog pending;
    if (failed_source) *failed_source = source_count;
    if (!catalog || !sources || !source_count)
        return RE_GUEST_DL_PROVIDER_E_ARGUMENT;
    if (source_count > RE_GUEST_DL_PROVIDER_MAX)
        return RE_GUEST_DL_PROVIDER_E_CAPACITY;

    memset(&pending, 0, sizeof(pending));
    for (size_t i = 0; i < source_count; ++i) {
        const REGUESTDLProviderSource *source = &sources[i];
        if (!valid_soname(source->soname)) {
            if (failed_source) *failed_source = i;
            return RE_GUEST_DL_PROVIDER_E_NAME;
        }
        if ((!source->symbols && source->symbol_count) ||
            source->symbol_count > RE_GUEST_DL_PROVIDER_MAX_SYMBOLS) {
            if (failed_source) *failed_source = i;
            return RE_GUEST_DL_PROVIDER_E_CAPACITY;
        }
        for (size_t j = 0; j < i; ++j) {
            if (strcmp(sources[j].soname, source->soname) == 0) {
                if (failed_source) *failed_source = i;
                return RE_GUEST_DL_PROVIDER_E_NAME;
            }
        }
        for (size_t j = 0; j < source->symbol_count; ++j) {
            if (!valid_export(&source->symbols[j], source->soname)) {
                if (failed_source) *failed_source = i;
                return RE_GUEST_DL_PROVIDER_E_SYMBOL;
            }
        }
        pending.entries[i].soname = source->soname;
        pending.entries[i].symbols = source->symbols;
        pending.entries[i].symbol_count = source->symbol_count;
    }
    pending.count = source_count;
    *catalog = pending;
    return RE_GUEST_DL_PROVIDER_OK;
}

int re_guest_dl_provider_catalog_open_callback(
    const char *path, uint32_t flags, uintptr_t *native_handle, void *user) {
    REGUESTDLProviderCatalog *catalog = (REGUESTDLProviderCatalog *)user;
    (void)flags;
    if (native_handle) *native_handle = (uintptr_t)0;
    if (!catalog || !native_handle || !valid_soname(path)) return -1;
    for (size_t i = 0; i < catalog->count; ++i) {
        REGUESTDLProviderEntry *entry = &catalog->entries[i];
        if (strcmp(entry->soname, path) != 0) continue;
        if (entry->open_references == UINT32_MAX) return -1;
        ++entry->open_references;
        *native_handle = (uintptr_t)(i + 1u);
        return 0;
    }
    return -1;
}

int re_guest_dl_provider_catalog_symbol_callback(
    uintptr_t native_handle, const char *name,
    uint32_t *guest_address, void *user) {
    REGUESTDLProviderCatalog *catalog = (REGUESTDLProviderCatalog *)user;
    REGUESTDLProviderEntry *entry;
    REGUESTSYMBOLREQUEST request;
    REGUESTSYMBOLRESULT result;
    if (guest_address) *guest_address = 0;
    if (!catalog || !guest_address || !name || !name[0] ||
        strlen(name) >= RE_GUEST_DL_SYMBOL_CAP ||
        native_handle == (uintptr_t)0 ||
        native_handle > (uintptr_t)catalog->count)
        return -1;
    entry = &catalog->entries[(size_t)native_handle - 1u];
    if (!entry->open_references) return -1;

    request.name = name;
    request.version_name = NULL;
    request.version_library = NULL;
    request.allow_unversioned_fallback = 0u;
    if (re_guest_symbol_resolve(&request, entry->symbols,
            entry->symbol_count, &result) != RE_GUEST_SYMBOL_OK ||
        result.value == 0u)
        return -1;
    *guest_address = result.value;
    return 0;
}

int re_guest_dl_provider_catalog_close_callback(
    uintptr_t native_handle, void *user) {
    REGUESTDLProviderCatalog *catalog = (REGUESTDLProviderCatalog *)user;
    REGUESTDLProviderEntry *entry;
    if (!catalog || native_handle == (uintptr_t)0 ||
        native_handle > (uintptr_t)catalog->count)
        return -1;
    entry = &catalog->entries[(size_t)native_handle - 1u];
    if (!entry->open_references) return -1;
    --entry->open_references;
    return 0;
}
