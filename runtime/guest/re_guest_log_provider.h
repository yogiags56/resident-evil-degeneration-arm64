#ifndef RE_GUEST_LOG_PROVIDER_H
#define RE_GUEST_LOG_PROVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_LOG_TAG_CAP 128u
#define RE_GUEST_LOG_FORMAT_CAP 512u
#define RE_GUEST_LOG_MESSAGE_CAP 2048u

typedef int (*REGUESTLOGWriteCallback)(
    int priority, const char *tag, const char *message, void *user);

typedef struct {
    REGUESTLOGWriteCallback write_callback;
    void *user;
} REGUESTLOGProvider;

typedef enum {
    RE_GUEST_LOG_OK = 0,
    RE_GUEST_LOG_E_ARGUMENT = -940,
    RE_GUEST_LOG_E_SYMBOL = -941,
    RE_GUEST_LOG_E_CAPACITY = -942,
    RE_GUEST_LOG_E_IMPORT = -943,
    RE_GUEST_LOG_E_FORMAT = -944,
    RE_GUEST_LOG_E_GUEST_MEMORY = -945
} REGUESTLOGStatus;

/*
 * Bind only the two relocation-backed liblog imports in the supplied ELF32
 * dynamic-symbol inventory. The symbol table indices become import IDs.
 * Output exports contain guest Thumb trampoline addresses and are suitable
 * for an exact-SONAME virtual "liblog.so" provider.
 *
 * Imports and export output are updated transactionally. On failure the
 * import table and export output remain unchanged.
 */
REGUESTLOGStatus re_guest_log_provider_bind_elf_symbols(
    REGUESTLOGProvider *provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols,
    size_t symbol_count,
    REGUESTSYMBOL *exports,
    size_t export_capacity,
    size_t *export_count,
    size_t *failed_symbol_index);

#ifdef __cplusplus
}
#endif
#endif
