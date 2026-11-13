#ifndef RE_GUEST_LIBC_SSCANF_PROVIDER_H
#define RE_GUEST_LIBC_SSCANF_PROVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_LIBC_SSCANF_SYMBOL_COUNT 1u
#define RE_GUEST_LIBC_SSCANF_MAX_BYTES 2048u

typedef struct {
    uint8_t initialized;
} REGUESTLIBCSSCANFProvider;

typedef enum {
    RE_GUEST_LIBC_SSCANF_OK = 0,
    RE_GUEST_LIBC_SSCANF_E_ARGUMENT = -1100,
    RE_GUEST_LIBC_SSCANF_E_SYMBOL = -1101,
    RE_GUEST_LIBC_SSCANF_E_CAPACITY = -1102,
    RE_GUEST_LIBC_SSCANF_E_IMPORT = -1103
} REGUESTLIBCSSCANFStatus;

REGUESTLIBCSSCANFStatus re_guest_libc_sscanf_provider_init(
    REGUESTLIBCSSCANFProvider *provider);

REGUESTLIBCSSCANFStatus re_guest_libc_sscanf_provider_bind_elf_symbols(
    REGUESTLIBCSSCANFProvider *provider,
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
