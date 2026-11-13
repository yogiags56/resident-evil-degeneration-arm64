#ifndef RE_GUEST_LIBC_FORMAT_PROVIDER_H
#define RE_GUEST_LIBC_FORMAT_PROVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT 2u

typedef struct REGUESTLIBCFormatProvider REGUESTLIBCFormatProvider;

typedef struct {
    uint32_t kind;
    REGUESTLIBCFormatProvider *provider;
} REGUESTLIBCFormatContext;

struct REGUESTLIBCFormatProvider {
    uint8_t initialized;
    REGUESTLIBCFormatContext contexts[RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT];
};

typedef enum {
    RE_GUEST_LIBC_FORMAT_OK = 0,
    RE_GUEST_LIBC_FORMAT_E_ARGUMENT = -1080,
    RE_GUEST_LIBC_FORMAT_E_SYMBOL = -1081,
    RE_GUEST_LIBC_FORMAT_E_CAPACITY = -1082,
    RE_GUEST_LIBC_FORMAT_E_IMPORT = -1083
} REGUESTLIBCFormatStatus;

REGUESTLIBCFormatStatus re_guest_libc_format_provider_init(
    REGUESTLIBCFormatProvider *provider);

REGUESTLIBCFormatStatus re_guest_libc_format_provider_bind_elf_symbols(
    REGUESTLIBCFormatProvider *provider,
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
