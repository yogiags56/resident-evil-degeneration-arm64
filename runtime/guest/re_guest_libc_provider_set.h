#ifndef RE_GUEST_LIBC_PROVIDER_SET_H
#define RE_GUEST_LIBC_PROVIDER_SET_H

#include <stddef.h>

#include "re_guest_libc_alloc_provider.h"
#include "re_guest_libc_format_provider.h"
#include "re_guest_libc_stdio_provider.h"
#include "re_guest_libc_string_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT \
    (RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT + RE_GUEST_LIBC_STRING_SYMBOL_COUNT + \
     RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT + RE_GUEST_LIBC_STDIO_SYMBOL_COUNT)

typedef struct {
    REGUESTLIBCAllocProvider *alloc;
    REGUESTLIBCStringProvider *string;
    REGUESTLIBCFormatProvider *format;
    REGUESTLIBCStdioProvider *stdio;
} REGUESTLIBCProviderSet;

typedef enum {
    RE_GUEST_LIBC_PROVIDER_SET_OK = 0,
    RE_GUEST_LIBC_PROVIDER_SET_E_ARGUMENT = -1100,
    RE_GUEST_LIBC_PROVIDER_SET_E_CAPACITY = -1101,
    RE_GUEST_LIBC_PROVIDER_SET_E_ALLOC = -1102,
    RE_GUEST_LIBC_PROVIDER_SET_E_STRING = -1103,
    RE_GUEST_LIBC_PROVIDER_SET_E_FORMAT = -1104,
    RE_GUEST_LIBC_PROVIDER_SET_E_STDIO = -1105
} REGUESTLIBCProviderSetStatus;

/* Bind the four implemented libc provider families against one libc.so
 * dynamic-symbol table. Import-table changes and output exports are committed
 * together: any provider failure restores the original import table and
 * leaves the caller's export array untouched.
 *
 * Providers must already be initialized and remain alive for as long as the
 * import table can dispatch their handlers. The caller owns all storage.
 */
REGUESTLIBCProviderSetStatus re_guest_libc_provider_set_bind_elf_symbols(
    const REGUESTLIBCProviderSet *set, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index);

#ifdef __cplusplus
}
#endif
#endif
