#ifndef RE_GUEST_DL_LIBC_CATALOG_H
#define RE_GUEST_DL_LIBC_CATALOG_H

#include "re_guest_dl_provider_catalog.h"
#include "re_guest_libc_provider_set.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Extend an existing immutable DSO catalog with the implemented libc.so
 * guest-callable imports. The libc symbol table is bound through the shared
 * import dispatcher, then published as an exact-SONAME catalog entry.
 *
 * module_sources and their symbol storage remain caller-owned. libc_exports
 * must also remain alive for the catalog lifetime. The import table, catalog,
 * and libc_exports are committed together only when all operations succeed.
 * A libc.so entry must not already exist in module_sources.
 */
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
    size_t *failed_module);

#ifdef __cplusplus
}
#endif
#endif
