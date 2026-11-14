#include "re_guest_dl_libc_catalog.h"

#include <string.h>

REGUESTDLProviderStatus re_guest_dl_provider_catalog_init_with_libc_log_zlib_dl_with_cxx(
    REGUESTDLProviderCatalog *catalog,
    REGUESTDLBridge *dl_bridge,
    const REGUESTDLProviderSource *module_sources,
    size_t module_count,
    const REGUESTLIBCProviderSet *libc_set,
    REGUESTLOGProvider *log_provider,
    REGUESTZLIBProvider *zlib_provider,
    REGUESTCXXHeapProvider *cxx_provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *consumer_symbols,
    size_t consumer_symbol_count,
    REGUESTSYMBOL *libc_exports,
    size_t libc_export_capacity,
    size_t *libc_export_count,
    REGUESTSYMBOL *log_exports,
    size_t log_export_capacity,
    size_t *log_export_count,
    REGUESTSYMBOL *zlib_exports,
    size_t zlib_export_capacity,
    size_t *zlib_export_count,
    REGUESTSYMBOL *dl_exports,
    size_t dl_export_capacity,
    size_t *dl_export_count,
    REGUESTSYMBOL *cxx_exports,
    size_t cxx_export_capacity,
    size_t *cxx_export_count,
    uint32_t error_guest_address,
    size_t *failed_symbol_index,
    size_t *failed_module) {
    REGUESTDLProviderSource sources[RE_GUEST_DL_PROVIDER_MAX];
    REGUESTDLProviderCatalog pending;
    REGUESTDLBridge pending_bridge;
    REGUESTIMPORTTable import_backup;
    REGUESTSYMBOL libc_pending[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    REGUESTSYMBOL log_pending[2];
    REGUESTSYMBOL zlib_pending[4];
    REGUESTSYMBOL dl_pending[4];
    REGUESTSYMBOL cxx_pending[4];
    REGUESTDLImportPointers dl_pointers;
    size_t libc_count = 0u, log_count = 0u, zlib_count = 0u, dl_count = 0u, cxx_count = 0u;
    size_t failed_symbol = consumer_symbol_count, failed_source = module_count;
    REGUESTLIBCProviderSetStatus libc_status;
    REGUESTLOGStatus log_status;
    REGUESTZLIBStatus zlib_status;
    REGUESTDLStatus dl_status;
    REGUESTCXXHeapStatus cxx_status;

    if (libc_export_count) *libc_export_count = 0u;
    if (log_export_count) *log_export_count = 0u;
    if (zlib_export_count) *zlib_export_count = 0u;
    if (dl_export_count) *dl_export_count = 0u;
    if (cxx_export_count) *cxx_export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = consumer_symbol_count;
    if (failed_module) *failed_module = module_count;
    if (!catalog || !dl_bridge || (!module_sources && module_count) ||
        !libc_set || !log_provider || !zlib_provider || !cxx_provider || !imports ||
        !consumer_symbols || !libc_exports || !log_exports || !zlib_exports ||
        !dl_exports || !cxx_exports || !libc_export_count || !log_export_count ||
        !zlib_export_count || !dl_export_count || !cxx_export_count)
        return RE_GUEST_DL_PROVIDER_E_ARGUMENT;
    if (module_count > RE_GUEST_DL_PROVIDER_MAX - 5u ||
        libc_export_capacity < RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT ||
        log_export_capacity < 2u || zlib_export_capacity < 4u ||
        dl_export_capacity < 4u || cxx_export_capacity < 4u)
        return RE_GUEST_DL_PROVIDER_E_CAPACITY;

    for (size_t i = 0; i < module_count; ++i) {
        if (!module_sources[i].soname ||
            strcmp(module_sources[i].soname, "libc.so") == 0 ||
            strcmp(module_sources[i].soname, "liblog.so") == 0 ||
            strcmp(module_sources[i].soname, "libz.so") == 0 ||
            strcmp(module_sources[i].soname, "libdl.so") == 0 ||
            strcmp(module_sources[i].soname, "libstdc++.so") == 0) {
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
    zlib_status = re_guest_zlib_provider_bind_elf_symbols(
        zlib_provider, imports, consumer_symbols, consumer_symbol_count,
        zlib_pending, 4u, &zlib_count, &failed_symbol);
    if (zlib_status != RE_GUEST_ZLIB_OK) {
        *imports = import_backup;
        if (failed_symbol_index) *failed_symbol_index = failed_symbol;
        return RE_GUEST_DL_PROVIDER_E_LIBZ;
    }

    /* Initialize a private bridge that points at the caller's catalog object.
       Its user pointer remains valid after the pending catalog is committed. */
    dl_status = re_guest_dl_bridge_init(
        &pending_bridge,
        re_guest_dl_provider_catalog_open_callback,
        re_guest_dl_provider_catalog_symbol_callback,
        re_guest_dl_provider_catalog_close_callback,
        catalog,
        error_guest_address);
    if (dl_status == RE_GUEST_DL_OK)
        dl_status = re_guest_dl_bridge_bind_elf_symbols(
            &pending_bridge, imports, consumer_symbols, consumer_symbol_count,
            &dl_pointers, &failed_symbol);
    if (dl_status == RE_GUEST_DL_OK)
        dl_status = re_guest_dl_bridge_build_provider_symbols(
            "libdl.so", &dl_pointers, dl_pending, 4u, &dl_count);
    if (dl_status != RE_GUEST_DL_OK) {
        *imports = import_backup;
        if (failed_symbol_index) *failed_symbol_index = failed_symbol;
        return RE_GUEST_DL_PROVIDER_E_LIBDL;
    }
    cxx_status = re_guest_cxx_heap_provider_bind_elf_symbols(
        cxx_provider, imports, consumer_symbols, consumer_symbol_count,
        cxx_pending, 4u, &cxx_count, &failed_symbol);
    if (cxx_status != RE_GUEST_CXX_HEAP_OK) {
        *imports = import_backup;
        if (failed_symbol_index) *failed_symbol_index = failed_symbol;
        return RE_GUEST_DL_PROVIDER_E_LIBSTDCXX;
    }

    sources[module_count].soname = "libc.so";
    sources[module_count].symbols = libc_pending;
    sources[module_count].symbol_count = libc_count;
    sources[module_count + 1u].soname = "liblog.so";
    sources[module_count + 1u].symbols = log_pending;
    sources[module_count + 1u].symbol_count = log_count;
    sources[module_count + 2u].soname = "libz.so";
    sources[module_count + 2u].symbols = zlib_pending;
    sources[module_count + 2u].symbol_count = zlib_count;
    sources[module_count + 3u].soname = "libdl.so";
    sources[module_count + 3u].symbols = dl_pending;
    sources[module_count + 3u].symbol_count = dl_count;
    sources[module_count + 4u].soname = "libstdc++.so";
    sources[module_count + 4u].symbols = cxx_pending;
    sources[module_count + 4u].symbol_count = cxx_count;
    {
        REGUESTDLProviderStatus status = re_guest_dl_provider_catalog_init(
            &pending, sources, module_count + 5u, &failed_source);
        if (status != RE_GUEST_DL_PROVIDER_OK) {
            *imports = import_backup;
            if (failed_module) *failed_module = failed_source;
            return status;
        }
    }

    memcpy(libc_exports, libc_pending, libc_count * sizeof(libc_pending[0]));
    memcpy(log_exports, log_pending, log_count * sizeof(log_pending[0]));
    memcpy(zlib_exports, zlib_pending, zlib_count * sizeof(zlib_pending[0]));
    memcpy(dl_exports, dl_pending, dl_count * sizeof(dl_pending[0]));
    memcpy(cxx_exports, cxx_pending, cxx_count * sizeof(cxx_pending[0]));
    pending.entries[module_count].symbols = libc_exports;
    pending.entries[module_count + 1u].symbols = log_exports;
    pending.entries[module_count + 2u].symbols = zlib_exports;
    pending.entries[module_count + 3u].symbols = dl_exports;
    pending.entries[module_count + 4u].symbols = cxx_exports;
    *catalog = pending;
    *dl_bridge = pending_bridge;
    *libc_export_count = libc_count;
    *log_export_count = log_count;
    *zlib_export_count = zlib_count;
    *dl_export_count = dl_count;
    *cxx_export_count = cxx_count;
    if (failed_symbol_index) *failed_symbol_index = consumer_symbol_count;
    if (failed_module) *failed_module = module_count + 5u;
    return RE_GUEST_DL_PROVIDER_OK;
}
