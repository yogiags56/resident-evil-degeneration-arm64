#ifndef RE_GUEST_DL_ELF32_ADAPTER_H
#define RE_GUEST_DL_ELF32_ADAPTER_H
#include "re_guest_dl_provider_catalog.h"
#include "re_guest_elf32_provider_catalog.h"
#ifdef __cplusplus
extern "C" {
#endif
REGUESTDLProviderStatus re_guest_dl_provider_catalog_init_from_elf32(
    REGUESTDLProviderCatalog *catalog,
    const REGUESTELF32PROVIDERMODULE *modules,
    size_t module_count,
    REGUESTSYMBOL *symbol_storage,
    size_t symbol_capacity,
    size_t *symbol_count,
    size_t *failed_module,
    size_t *failed_symbol);
#ifdef __cplusplus
}
#endif
#endif
