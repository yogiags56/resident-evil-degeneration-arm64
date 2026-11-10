#include "re_guest_dl_elf32_adapter.h"
#include <string.h>

/*
 * symbol_storage is caller-owned scratch and must outlive catalog handles.
 * It may be modified on failure; catalog is committed only after all modules
 * and SONAMEs validate.
 */
REGUESTDLProviderStatus re_guest_dl_provider_catalog_init_from_elf32(
    REGUESTDLProviderCatalog *catalog,
    const REGUESTELF32PROVIDERMODULE *modules,
    size_t module_count,
    REGUESTSYMBOL *symbol_storage,
    size_t symbol_capacity,
    size_t *symbol_count,
    size_t *failed_module,
    size_t *failed_symbol) {
    REGUESTDLProviderSource sources[RE_GUEST_DL_PROVIDER_MAX];
    REGUESTDLProviderCatalog pending;
    size_t used = 0;

    if (symbol_count) *symbol_count = 0;
    if (failed_module) *failed_module = 0;
    if (failed_symbol) *failed_symbol = 0;
    if (!catalog || !modules || !module_count || !symbol_count)
        return RE_GUEST_DL_PROVIDER_E_ARGUMENT;
    if (module_count > RE_GUEST_DL_PROVIDER_MAX)
        return RE_GUEST_DL_PROVIDER_E_CAPACITY;
    if (!symbol_storage && symbol_capacity)
        return RE_GUEST_DL_PROVIDER_E_ARGUMENT;

    memset(sources, 0, sizeof(sources));
    for (size_t i = 0; i < module_count; ++i) {
        size_t collected = 0;
        size_t local_module = 0;
        size_t local_symbol = 0;
        REGUESTELF32PROVIDERMODULE one = modules[i];
        REGUESTELF32PROVIDERStatus status =
            re_guest_elf32_collect_providers(
                &one, 1u,
                symbol_storage ? symbol_storage + used : NULL,
                symbol_capacity - used,
                &collected, &local_module, &local_symbol);
        if (status != RE_GUEST_ELF32_PROVIDER_OK) {
            if (failed_module) *failed_module = i;
            if (failed_symbol) *failed_symbol = local_symbol;
            return RE_GUEST_DL_PROVIDER_E_ELF32;
        }
        sources[i].soname = modules[i].soname;
        sources[i].symbols = collected ? symbol_storage + used : NULL;
        sources[i].symbol_count = collected;
        used += collected;
    }

    {
        size_t failed_source = module_count;
        REGUESTDLProviderStatus status =
            re_guest_dl_provider_catalog_init(
                &pending, sources, module_count, &failed_source);
        if (status != RE_GUEST_DL_PROVIDER_OK) {
            if (failed_module) *failed_module = failed_source;
            return status;
        }
    }

    *catalog = pending;
    *symbol_count = used;
    if (failed_module) *failed_module = module_count;
    if (failed_symbol) *failed_symbol = 0;
    return RE_GUEST_DL_PROVIDER_OK;
}
