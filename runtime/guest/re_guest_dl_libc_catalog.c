#include "re_guest_dl_libc_catalog.h"

#include <string.h>

REGUESTDLProviderStatus re_guest_dl_provider_catalog_init_with_libc_set(
    REGUESTDLProviderCatalog *catalog,
    const REGUESTDLProviderSource *module_sources,
    size_t module_count,
    const REGUESTLIBCProviderSet *libc_set,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *libc_dynamic_symbols,
    size_t libc_symbol_count,
    REGUESTSYMBOL *libc_exports,
    size_t libc_export_capacity,
    size_t *libc_export_count,
    size_t *failed_symbol_index,
    size_t *failed_module) {
    REGUESTDLProviderSource sources[RE_GUEST_DL_PROVIDER_MAX];
    REGUESTDLProviderCatalog pending;
    REGUESTIMPORTTable import_backup;
    REGUESTSYMBOL export_pending[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    size_t export_count = 0u, failed_symbol = libc_symbol_count;
    size_t failed_source = module_count;
    REGUESTLIBCProviderSetStatus libc_status;

    if (libc_export_count) *libc_export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = libc_symbol_count;
    if (failed_module) *failed_module = module_count;
    if (!catalog || (!module_sources && module_count) || !libc_set ||
        !imports || !libc_dynamic_symbols || !libc_exports ||
        !libc_export_count)
        return RE_GUEST_DL_PROVIDER_E_ARGUMENT;
    if (module_count >= RE_GUEST_DL_PROVIDER_MAX ||
        libc_export_capacity < RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT)
        return RE_GUEST_DL_PROVIDER_E_CAPACITY;

    for (size_t i = 0; i < module_count; ++i) {
        if (!module_sources[i].soname ||
            strcmp(module_sources[i].soname, "libc.so") == 0) {
            if (failed_module) *failed_module = i;
            return RE_GUEST_DL_PROVIDER_E_NAME;
        }
        sources[i] = module_sources[i];
    }

    import_backup = *imports;
    libc_status = re_guest_libc_provider_set_bind_elf_symbols(
        libc_set, imports, libc_dynamic_symbols, libc_symbol_count,
        export_pending, RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,
        &export_count, &failed_symbol);
    if (libc_status != RE_GUEST_LIBC_PROVIDER_SET_OK) {
        *imports = import_backup;
        if (failed_symbol_index) *failed_symbol_index = failed_symbol;
        return RE_GUEST_DL_PROVIDER_E_LIBC;
    }

    sources[module_count].soname = "libc.so";
    sources[module_count].symbols = export_pending;
    sources[module_count].symbol_count = export_count;
    {
        REGUESTDLProviderStatus status = re_guest_dl_provider_catalog_init(
            &pending, sources, module_count + 1u, &failed_source);
        if (status != RE_GUEST_DL_PROVIDER_OK) {
            *imports = import_backup;
            if (failed_module) *failed_module = failed_source;
            return status;
        }
    }

    memcpy(libc_exports, export_pending, export_count * sizeof(export_pending[0]));
    pending.entries[module_count].symbols = libc_exports;
    *catalog = pending;
    *libc_export_count = export_count;
    if (failed_symbol_index) *failed_symbol_index = libc_symbol_count;
    if (failed_module) *failed_module = module_count + 1u;
    return RE_GUEST_DL_PROVIDER_OK;
}
