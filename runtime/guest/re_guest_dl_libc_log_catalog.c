#include "re_guest_dl_libc_catalog.h"

#include <string.h>

REGUESTDLProviderStatus re_guest_dl_provider_catalog_init_with_libc_and_log(
    REGUESTDLProviderCatalog *catalog,
    const REGUESTDLProviderSource *module_sources,
    size_t module_count,
    const REGUESTLIBCProviderSet *libc_set,
    REGUESTLOGProvider *log_provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *consumer_symbols,
    size_t consumer_symbol_count,
    REGUESTSYMBOL *libc_exports,
    size_t libc_export_capacity,
    size_t *libc_export_count,
    REGUESTSYMBOL *log_exports,
    size_t log_export_capacity,
    size_t *log_export_count,
    size_t *failed_symbol_index,
    size_t *failed_module) {
    REGUESTDLProviderSource sources[RE_GUEST_DL_PROVIDER_MAX];
    REGUESTDLProviderCatalog pending;
    REGUESTIMPORTTable import_backup;
    REGUESTSYMBOL libc_pending[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTSYMBOL log_pending[2];
    size_t libc_count = 0u, log_count = 0u;
    size_t failed_symbol = consumer_symbol_count, failed_source = module_count;
    REGUESTLIBCProviderSetStatus libc_status;
    REGUESTLOGStatus log_status;

    if (libc_export_count) *libc_export_count = 0u;
    if (log_export_count) *log_export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = consumer_symbol_count;
    if (failed_module) *failed_module = module_count;
    if (!catalog || (!module_sources && module_count) || !libc_set ||
        !log_provider || !imports || !consumer_symbols || !libc_exports ||
        !log_exports || !libc_export_count || !log_export_count)
        return RE_GUEST_DL_PROVIDER_E_ARGUMENT;
    if (module_count > RE_GUEST_DL_PROVIDER_MAX - 2u ||
        libc_export_capacity < RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT ||
        log_export_capacity < 2u)
        return RE_GUEST_DL_PROVIDER_E_CAPACITY;

    for (size_t i = 0; i < module_count; ++i) {
        if (!module_sources[i].soname ||
            strcmp(module_sources[i].soname, "libc.so") == 0 ||
            strcmp(module_sources[i].soname, "liblog.so") == 0) {
            if (failed_module) *failed_module = i;
            return RE_GUEST_DL_PROVIDER_E_NAME;
        }
        sources[i] = module_sources[i];
    }

    import_backup = *imports;
    libc_status = re_guest_libc_provider_set_bind_elf_symbols(
        libc_set, imports, consumer_symbols, consumer_symbol_count,
        libc_pending, RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT,
        &libc_count, &failed_symbol);
    if (libc_status != RE_GUEST_LIBC_PROVIDER_SET_OK) {
        *imports = import_backup;
        if (failed_symbol_index) *failed_symbol_index = failed_symbol;
        return RE_GUEST_DL_PROVIDER_E_LIBC;
    }

    log_status = re_guest_log_provider_bind_elf_symbols(
        log_provider, imports, consumer_symbols, consumer_symbol_count,
        log_pending, 2u, &log_count, &failed_symbol);
    if (log_status != RE_GUEST_LOG_OK) {
        *imports = import_backup;
        if (failed_symbol_index) *failed_symbol_index = failed_symbol;
        return RE_GUEST_DL_PROVIDER_E_LIBLOG;
    }

    sources[module_count].soname = "libc.so";
    sources[module_count].symbols = libc_pending;
    sources[module_count].symbol_count = libc_count;
    sources[module_count + 1u].soname = "liblog.so";
    sources[module_count + 1u].symbols = log_pending;
    sources[module_count + 1u].symbol_count = log_count;
    {
        REGUESTDLProviderStatus status = re_guest_dl_provider_catalog_init(
            &pending, sources, module_count + 2u, &failed_source);
        if (status != RE_GUEST_DL_PROVIDER_OK) {
            *imports = import_backup;
            if (failed_module) *failed_module = failed_source;
            return status;
        }
    }

    memcpy(libc_exports, libc_pending, libc_count * sizeof(libc_pending[0]));
    memcpy(log_exports, log_pending, log_count * sizeof(log_pending[0]));
    pending.entries[module_count].symbols = libc_exports;
    pending.entries[module_count + 1u].symbols = log_exports;
    *catalog = pending;
    *libc_export_count = libc_count;
    *log_export_count = log_count;
    if (failed_symbol_index) *failed_symbol_index = consumer_symbol_count;
    if (failed_module) *failed_module = module_count + 2u;
    return RE_GUEST_DL_PROVIDER_OK;
}
