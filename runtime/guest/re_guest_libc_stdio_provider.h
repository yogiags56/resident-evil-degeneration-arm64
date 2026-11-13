#ifndef RE_GUEST_LIBC_STDIO_PROVIDER_H
#define RE_GUEST_LIBC_STDIO_PROVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_imports.h"
#include "re_guest_elf32_link.h"
#include "re_guest_symbol_resolve.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_LIBC_STDIO_SYMBOL_COUNT 13u
#define RE_GUEST_LIBC_STDIO_MAX_FILES 32u
#define RE_GUEST_LIBC_STDIO_MAX_PATH 512u
#define RE_GUEST_LIBC_STDIO_MAX_MODE 8u
#define RE_GUEST_LIBC_STDIO_TOKEN_BASE 0xf17e0000u
#define RE_GUEST_LIBC_STDIO_BUFFER_CAP 2048u

typedef struct {
    void *(*open)(void *user, const char *path, const char *mode);
    int64_t (*read)(void *user, void *stream, void *dst, uint64_t bytes);
    int64_t (*write)(void *user, void *stream, const void *src, uint64_t bytes);
    int (*seek)(void *user, void *stream, int64_t offset, int whence);
    int64_t (*tell)(void *user, void *stream);
    int (*close)(void *user, void *stream);
    int (*flush)(void *user, void *stream); /* Optional; NULL means unbuffered. */
} REGUESTLIBCStdioBackend;

typedef struct {
    void *stream;
    uint16_t generation;
    uint8_t active;
    uint8_t readable;
    uint8_t writable;
    uint8_t append;
    uint8_t eof;
    uint8_t error;
    uint16_t buffered_size;
    uint8_t buffered[RE_GUEST_LIBC_STDIO_BUFFER_CAP];
} REGUESTLIBCStdioFile;

typedef struct REGUESTLIBCStdioProvider REGUESTLIBCStdioProvider;
typedef struct {
    uint32_t kind;
    REGUESTLIBCStdioProvider *provider;
} REGUESTLIBCStdioContext;

struct REGUESTLIBCStdioProvider {
    REGUESTLIBCStdioBackend backend;
    void *backend_user;
    uint16_t next_generation;
    uint8_t initialized;
    REGUESTLIBCStdioFile files[RE_GUEST_LIBC_STDIO_MAX_FILES];
    REGUESTLIBCStdioContext contexts[RE_GUEST_LIBC_STDIO_SYMBOL_COUNT];
};

typedef enum {
    RE_GUEST_LIBC_STDIO_OK = 0,
    RE_GUEST_LIBC_STDIO_E_ARGUMENT = -1090,
    RE_GUEST_LIBC_STDIO_E_SYMBOL = -1091,
    RE_GUEST_LIBC_STDIO_E_CAPACITY = -1092,
    RE_GUEST_LIBC_STDIO_E_IMPORT = -1093
} REGUESTLIBCStdioStatus;

REGUESTLIBCStdioStatus re_guest_libc_stdio_provider_init(
    REGUESTLIBCStdioProvider *provider,
    const REGUESTLIBCStdioBackend *backend, void *backend_user);
REGUESTLIBCStdioStatus re_guest_libc_stdio_provider_bind_elf_symbols(
    REGUESTLIBCStdioProvider *provider, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index);
void re_guest_libc_stdio_provider_close_all(REGUESTLIBCStdioProvider *provider);

#ifdef __cplusplus
}
#endif
#endif
