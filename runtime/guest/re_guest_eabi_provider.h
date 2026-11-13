#ifndef RE_GUEST_EABI_PROVIDER_H
#define RE_GUEST_EABI_PROVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_EABI_SYMBOL_COUNT 21u

typedef struct {
    uint8_t initialized;
} REGUESTEABIProvider;

typedef enum {
    RE_GUEST_EABI_OK = 0,
    RE_GUEST_EABI_E_ARGUMENT = -1000,
    RE_GUEST_EABI_E_SYMBOL = -1001,
    RE_GUEST_EABI_E_CAPACITY = -1002,
    RE_GUEST_EABI_E_IMPORT = -1003
} REGUESTEABIStatus;

REGUESTEABIStatus re_guest_eabi_provider_init(REGUESTEABIProvider *provider);

REGUESTEABIStatus re_guest_eabi_provider_bind_elf_symbols(
    REGUESTEABIProvider *provider,
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
