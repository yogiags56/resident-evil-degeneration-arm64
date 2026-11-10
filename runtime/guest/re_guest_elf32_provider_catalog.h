#ifndef RE_GUEST_ELF32_PROVIDER_CATALOG_H
#define RE_GUEST_ELF32_PROVIDER_CATALOG_H
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RE_GUEST_ELF32_MAX_PROVIDER_MODULES 32u
typedef struct {
    const char *soname;
    const REGUESTELF32LINKSYMBOL *symbols;
    size_t symbol_count;
    uint32_t load_bias;
} REGUESTELF32PROVIDERMODULE;
typedef enum {
    RE_GUEST_ELF32_PROVIDER_OK=0,
    RE_GUEST_ELF32_PROVIDER_E_ARGUMENT=-750,
    RE_GUEST_ELF32_PROVIDER_E_CAPACITY=-751,
    RE_GUEST_ELF32_PROVIDER_E_NAME=-752,
    RE_GUEST_ELF32_PROVIDER_E_SYMBOL=-753,
    RE_GUEST_ELF32_PROVIDER_E_RANGE=-754
} REGUESTELF32PROVIDERStatus;
/* Flatten already-decoded module dynamic symbols into a caller-owned
 * provider catalog. Modules must be supplied in the intended global-scope
 * order. Only defined, externally visible GLOBAL/WEAK non-TLS symbols are
 * exported. SHN_ABS values are not rebased; ordinary definitions are.
 * Output is transactional and remains unchanged on error.
 */
REGUESTELF32PROVIDERStatus re_guest_elf32_collect_providers(
    const REGUESTELF32PROVIDERMODULE *modules,size_t module_count,
    REGUESTSYMBOL *providers,size_t provider_capacity,
    size_t *provider_count,size_t *failed_module,size_t *failed_symbol);
#ifdef __cplusplus
}
#endif
#endif
