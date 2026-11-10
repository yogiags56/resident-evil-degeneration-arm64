#include "re_guest_dl_elf32_adapter.h"

#include <stdlib.h>
#include <string.h>

/*
 * symbol_storage is caller-owned committed storage and must outlive catalog
 * handles. All candidate exports are first built in private temporary storage.
 * A failed initialization never changes the live catalog or symbol_storage.
 * Reinitialization after success requires callers to quiesce/close old handles.
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
    size_t offsets[RE_GUEST_DL_PROVIDER_MAX] = {0};
    size_t counts[RE_GUEST_DL_PROVIDER_MAX] = {0};
    REGUESTSYMBOL *temporary;
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
    if (symbol_capacity > SIZE_MAX / sizeof(*temporary))
        return RE_GUEST_DL_PROVIDER_E_CAPACITY;

    temporary = (REGUESTSYMBOL *)malloc(
        symbol_capacity ? symbol_capacity * sizeof(*temporary)
                        : sizeof(*temporary));
    if (!temporary) return RE_GUEST_DL_PROVIDER_E_MEMORY;

    memset(sources, 0, sizeof(sources));
    for (size_t i = 0; i < module_count; ++i) {
        size_t collected = 0;
        size_t local_module = 0;
        size_t local_symbol = 0;
        REGUESTELF32PROVIDERMODULE one = modules[i];
        REGUESTELF32PROVIDERStatus status;

        offsets[i] = used;
        status = re_guest_elf32_collect_providers(
            &one, 1u, temporary + used, symbol_capacity - used,
            &collected, &local_module, &local_symbol);
        if (status != RE_GUEST_ELF32_PROVIDER_OK) {
            if (failed_module) *failed_module = i;
            if (failed_symbol) *failed_symbol = local_symbol;
            free(temporary);
            return RE_GUEST_DL_PROVIDER_E_ELF32;
        }

        counts[i] = collected;
        sources[i].soname = modules[i].soname;
        sources[i].symbols = collected ? temporary + used : NULL;
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
            free(temporary);
            return status;
        }
    }

    if (used) {
        if (!symbol_storage) {
            free(temporary);
            return RE_GUEST_DL_PROVIDER_E_CAPACITY;
        }
        memcpy(symbol_storage, temporary, used * sizeof(*temporary));
    }
    for (size_t i = 0; i < module_count; ++i) {
        pending.entries[i].symbols =
            counts[i] ? symbol_storage + offsets[i] : NULL;
    }

    *catalog = pending;
    *symbol_count = used;
    if (failed_module) *failed_module = module_count;
    if (failed_symbol) *failed_symbol = 0;
    free(temporary);
    return RE_GUEST_DL_PROVIDER_OK;
}
