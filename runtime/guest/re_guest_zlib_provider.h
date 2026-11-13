#ifndef RE_GUEST_ZLIB_PROVIDER_H
#define RE_GUEST_ZLIB_PROVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_ZLIB_MAX_STREAMS 16u
#define RE_GUEST_ZLIB_STREAM32_SIZE 56u
#define RE_GUEST_ZLIB_IO_CHUNK_CAP 32768u

typedef struct {
    uint32_t guest_stream;
    uint32_t guest_state_token;
    void *native_state;
    uint8_t active;
    uint8_t busy;
} REGUESTZLIBStreamSlot;

typedef struct {
    REGUESTZLIBStreamSlot streams[RE_GUEST_ZLIB_MAX_STREAMS];
    uint32_t token_base;
} REGUESTZLIBProvider;

typedef enum {
    RE_GUEST_ZLIB_OK = 0,
    RE_GUEST_ZLIB_E_ARGUMENT = -960,
    RE_GUEST_ZLIB_E_SYMBOL = -961,
    RE_GUEST_ZLIB_E_CAPACITY = -962,
    RE_GUEST_ZLIB_E_IMPORT = -963,
    RE_GUEST_ZLIB_E_MEMORY = -964
} REGUESTZLIBStatus;

/* token_base is a 4-byte-aligned guest-only opaque state-token range. */
REGUESTZLIBStatus re_guest_zlib_provider_init(
    REGUESTZLIBProvider *provider, uint32_t token_base);

/* Binds exactly inflate, inflateEnd, inflateInit2_, and inflateInit_ from
 * the supplied ELF32 dynamic-symbol inventory as guest Thumb trampolines.
 * The provider object must outlive every guest zlib stream and import call.
 */
REGUESTZLIBStatus re_guest_zlib_provider_bind_elf_symbols(
    REGUESTZLIBProvider *provider,
    REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols,
    size_t symbol_count,
    REGUESTSYMBOL *exports,
    size_t export_capacity,
    size_t *export_count,
    size_t *failed_symbol_index);

/* Release every active native zlib stream. Call only after guest execution
 * has stopped and before destroying the provider.
 */
void re_guest_zlib_provider_shutdown(REGUESTZLIBProvider *provider);

#ifdef __cplusplus
}
#endif
#endif
