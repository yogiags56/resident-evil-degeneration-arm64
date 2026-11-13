#ifndef RE_GUEST_LIBC_STRING_PROVIDER_H
#define RE_GUEST_LIBC_STRING_PROVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_LIBC_STRING_SYMBOL_COUNT 15u
#define RE_GUEST_LIBC_STRING_MAX_BYTES 65536u

typedef struct REGUESTLIBCStringProvider REGUESTLIBCStringProvider;

typedef struct {
    uint32_t kind;
    REGUESTLIBCStringProvider *provider;
} REGUESTLIBCStringContext;

struct REGUESTLIBCStringProvider {
    uint32_t strtok_next;
    REGUESTLIBCStringContext contexts[RE_GUEST_LIBC_STRING_SYMBOL_COUNT];
    uint8_t initialized;
};

typedef enum {
    RE_GUEST_LIBC_STRING_OK = 0,
    RE_GUEST_LIBC_STRING_E_ARGUMENT = -1060,
    RE_GUEST_LIBC_STRING_E_SYMBOL = -1061,
    RE_GUEST_LIBC_STRING_E_CAPACITY = -1062,
    RE_GUEST_LIBC_STRING_E_IMPORT = -1063
} REGUESTLIBCStringStatus;

REGUESTLIBCStringStatus re_guest_libc_string_provider_init(
    REGUESTLIBCStringProvider *provider);

REGUESTLIBCStringStatus re_guest_libc_string_provider_bind_elf_symbols(
    REGUESTLIBCStringProvider *provider,
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
