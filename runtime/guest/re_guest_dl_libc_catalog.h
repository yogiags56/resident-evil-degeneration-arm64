#ifndef RE_GUEST_DL_LIBC_CATALOG_H
#define RE_GUEST_DL_LIBC_CATALOG_H

#include "re_guest_dl_provider_catalog.h"
#include "re_guest_libc_provider_set.h"
#include "re_guest_log_provider.h"
#include "re_guest_zlib_provider.h"
#include "re_guest_dl_provider_bridge.h"
#include "re_guest_cxx_heap_provider.h"

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

/* Extend the catalog with both implemented libc.so and liblog.so providers.
 * The same ELF32 consumer symbol inventory supplies relocation-backed imports
 * for both DSOs. The two export arrays, catalog, and import table are committed
 * atomically. Provider objects and export storage must outlive guest calls.
 */
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
    size_t *failed_module);

/* Extend the combined libc + liblog catalog with the implemented libz inflate
 * subset. The zlib provider must be initialized by the caller with a guest
 * token range before calling this initializer and must outlive guest streams.
 * Catalog, imports, and all three export arrays are committed atomically.
 */
REGUESTDLProviderStatus re_guest_dl_provider_catalog_init_with_libc_log_zlib(
    REGUESTDLProviderCatalog *catalog,
    const REGUESTDLProviderSource *module_sources,
    size_t module_count,
    const REGUESTLIBCProviderSet *libc_set,
    REGUESTLOGProvider *log_provider,
    REGUESTZLIBProvider *zlib_provider,
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
    size_t *failed_symbol_index,
    size_t *failed_module);

/* Extend libc + liblog + libz with the four relocation-backed libdl imports.
 * The bridge object is bound as the import-handler context and initialized
 * against the committed catalog before this function returns. The bridge,
 * provider objects, and all export arrays must outlive guest execution.
 */
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
    size_t *failed_module);

#ifdef __cplusplus
}
#endif
#endif
