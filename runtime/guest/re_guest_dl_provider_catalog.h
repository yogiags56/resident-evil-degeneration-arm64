#ifndef RE_GUEST_DL_PROVIDER_CATALOG_H
#define RE_GUEST_DL_PROVIDER_CATALOG_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_dl_bridge.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_DL_PROVIDER_MAX 16u
#define RE_GUEST_DL_PROVIDER_MAX_SYMBOLS 16384u

typedef struct {
    const char *soname;
    const REGUESTSYMBOL *symbols;
    size_t symbol_count;
} REGUESTDLProviderSource;

typedef struct {
    const char *soname;
    const REGUESTSYMBOL *symbols;
    size_t symbol_count;
    uint32_t open_references;
} REGUESTDLProviderEntry;

typedef struct {
    REGUESTDLProviderEntry entries[RE_GUEST_DL_PROVIDER_MAX];
    size_t count;
} REGUESTDLProviderCatalog;

typedef enum {
    RE_GUEST_DL_PROVIDER_OK = 0,
    RE_GUEST_DL_PROVIDER_E_ARGUMENT = -920,
    RE_GUEST_DL_PROVIDER_E_CAPACITY = -921,
    RE_GUEST_DL_PROVIDER_E_NAME = -922,
    RE_GUEST_DL_PROVIDER_E_SYMBOL = -923,
    RE_GUEST_DL_PROVIDER_E_ELF32 = -924,
    RE_GUEST_DL_PROVIDER_E_MEMORY = -925
} REGUESTDLProviderStatus;

/*
 * Build an immutable, caller-owned catalog of already-verified guest exports.
 * Source arrays and their strings must outlive the catalog and any open
 * REGUESTDLBridge handles that use it. Initialization is transactional.
 */
REGUESTDLProviderStatus re_guest_dl_provider_catalog_init(
    REGUESTDLProviderCatalog *catalog,
    const REGUESTDLProviderSource *sources,
    size_t source_count,
    size_t *failed_source);

/*
 * These three functions match REGUESTDLOpenCallback,
 * REGUESTDLSymbolCallback, and REGUESTDLCloseCallback respectively.
 * The uintptr_t handle is a host-side one-based catalog index, not a guest
 * address and not a native function pointer.
 *
 * Only exact SONAME matches are accepted. Paths/search directories are never
 * interpreted. dlsym returns only a caller-supplied 32-bit guest address.
 */
int re_guest_dl_provider_catalog_open_callback(
    const char *path, uint32_t flags, uintptr_t *native_handle, void *user);
int re_guest_dl_provider_catalog_symbol_callback(
    uintptr_t native_handle, const char *name,
    uint32_t *guest_address, void *user);
int re_guest_dl_provider_catalog_close_callback(
    uintptr_t native_handle, void *user);

#ifdef __cplusplus
}
#endif
#endif
