#ifndef RE_GUEST_CXX_HEAP_PROVIDER_H
#define RE_GUEST_CXX_HEAP_PROVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_heap.h"
#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_CXX_HEAP_ALIGNMENT RE_GUEST_HEAP_DEFAULT_ALIGNMENT

typedef struct {
    REGUESTHEAP heap;
} REGUESTCXXHeapProvider;

typedef enum {
    RE_GUEST_CXX_HEAP_OK = 0,
    RE_GUEST_CXX_HEAP_E_ARGUMENT = -980,
    RE_GUEST_CXX_HEAP_E_SYMBOL = -981,
    RE_GUEST_CXX_HEAP_E_CAPACITY = -982,
    RE_GUEST_CXX_HEAP_E_IMPORT = -983,
    RE_GUEST_CXX_HEAP_E_MEMORY = -984
} REGUESTCXXHeapStatus;

/* C++ new/delete share this heap arena with the libc malloc/free provider.
 * The arena is a dedicated guest RW mapping; it is never a host allocation.
 */
REGUESTCXXHeapStatus re_guest_cxx_heap_provider_init(
    REGUESTCXXHeapProvider *provider, uint32_t heap_base, uint32_t heap_size);

REGUESTCXXHeapStatus re_guest_cxx_heap_provider_bind_elf_symbols(
    REGUESTCXXHeapProvider *provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols,
    size_t symbol_count,
    REGUESTSYMBOL *exports,
    size_t export_capacity,
    size_t *export_count,
    size_t *failed_symbol_index);

/* Stop guest execution before shutdown. This clears host-side allocation
 * records; it does not unmap or modify the guest heap bytes.
 */
void re_guest_cxx_heap_provider_shutdown(REGUESTCXXHeapProvider *provider);

#ifdef __cplusplus
}
#endif
#endif
