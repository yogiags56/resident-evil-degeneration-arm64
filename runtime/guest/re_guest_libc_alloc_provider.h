#ifndef RE_GUEST_LIBC_ALLOC_PROVIDER_H
#define RE_GUEST_LIBC_ALLOC_PROVIDER_H

#include <stddef.h>

#include "re_guest_heap.h"
#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT 4u

typedef struct {
    REGUESTHEAP *heap;
} REGUESTLIBCAllocProvider;

typedef enum {
    RE_GUEST_LIBC_ALLOC_OK = 0,
    RE_GUEST_LIBC_ALLOC_E_ARGUMENT = -1040,
    RE_GUEST_LIBC_ALLOC_E_SYMBOL = -1041,
    RE_GUEST_LIBC_ALLOC_E_CAPACITY = -1042,
    RE_GUEST_LIBC_ALLOC_E_IMPORT = -1043
} REGUESTLIBCAllocStatus;

REGUESTLIBCAllocStatus re_guest_libc_alloc_provider_init(
    REGUESTLIBCAllocProvider *provider, REGUESTHEAP *heap);

REGUESTLIBCAllocStatus re_guest_libc_alloc_provider_bind_elf_symbols(
    REGUESTLIBCAllocProvider *provider,
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
