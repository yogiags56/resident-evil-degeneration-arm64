#include "re_guest_vfs_posix_imports.h"

#include <limits.h>
#include <string.h>
#include "s3e_aapcs32.h"

enum { POSIX_OPEN, POSIX_READ, POSIX_LSEEK, POSIX_CLOSE, POSIX_ACCESS };
static const char *const k_names[RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT] = {
    "open", "read", "lseek", "close", "access"
};

static void set_errno(REGUESTVFSPOSIXImportProvider *p,
                      const REGUESTMEMBUS *bus, int value) {
    p->last_errno = value;
    if (p->guest_errno_address && bus) {
        REGUESTMEMBUS writable = *bus;
        (void)re_guest_memory_write32(&writable, p->guest_errno_address,
                                      (uint32_t)value);
    }
}

static int guest_string(const REGUESTMEMBUS *bus, uint32_t address,
                        char out[RE_GUEST_VFS_POSIX_IMPORT_MAX_PATH]) {
    if (!bus || !bus->memory || !address || !out) return 0;
    for (uint32_t i = 0; i < RE_GUEST_VFS_POSIX_IMPORT_MAX_PATH; ++i) {
        uint64_t at = (uint64_t)address + i;
        uint8_t byte;
        if (at > UINT32_MAX ||
            re_guest_memory_read8(bus, (uint32_t)at, &byte) != RE_GUEST_MEM_OK)
            return 0;
        out[i] = (char)byte;
        if (!byte) return 1;
    }
    return 0;
}

static int map_open_error(int rc) {
    if (rc == RE_GUEST_VFS_POSIX_E_NOT_FOUND) return RE_GUEST_VFS_POSIX_ERRNO_ENOENT;
    if (rc == RE_GUEST_VFS_POSIX_E_ACCESS) return RE_GUEST_VFS_POSIX_ERRNO_EACCES;
    if (rc == RE_GUEST_VFS_POSIX_E_CAPACITY) return RE_GUEST_VFS_POSIX_ERRNO_EMFILE;
    if (rc == RE_GUEST_VFS_POSIX_E_ARGUMENT) return RE_GUEST_VFS_POSIX_ERRNO_EINVAL;
    return RE_GUEST_VFS_POSIX_ERRNO_EIO;
}

static REGUESTIMPORTStatus posix_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEMBUS *bus, void *user,
    REGUESTIMPORTResult *result) {
    REGUESTVFSPOSIXImportContext *ctx = (REGUESTVFSPOSIXImportContext *)user;
    REGUESTVFSPOSIXImportProvider *p;
    uint32_t a, b, c;
    if (!frame || !bus || !bus->memory || !ctx || !ctx->provider || !result)
        return RE_IMPORT_E_HANDLER;
    p = ctx->provider;
    if (!p->initialized || !p->filesystem || !p->filesystem->initialized)
        return RE_IMPORT_E_HANDLER;
    a = frame->r[0]; b = frame->r[1]; c = frame->r[2];
    result->r0 = result->r1 = result->r2 = result->r3 = 0u;
    set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_NONE);

    switch (ctx->kind) {
    case POSIX_OPEN: {
        char path[RE_GUEST_VFS_POSIX_IMPORT_MAX_PATH];
        int fd;
        if (!guest_string(bus, a, path)) {
            set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_EFAULT);
            result->r0 = (uint32_t)-1;
            break;
        }
        fd = re_guest_vfs_posix_open(p->filesystem, path, b);
        if (fd < 0) {
            set_errno(p, bus, map_open_error(fd));
            result->r0 = (uint32_t)-1;
        } else result->r0 = (uint32_t)fd;
        break;
    }
    case POSIX_READ: {
        void *host = NULL;
        int64_t got;
        if (c > INT32_MAX) {
            set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_EINVAL);
            result->r0 = (uint32_t)-1;
            break;
        }
        if (c && re_guest_memory_resolve(bus->memory, b, c,
                RE_GUEST_MEM_WRITE, &host) != RE_GUEST_MEM_OK) {
            set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_EFAULT);
            result->r0 = (uint32_t)-1;
            break;
        }
        got = re_guest_vfs_posix_read(p->filesystem, (int32_t)a,
                                      host, c);
        if (got < 0) {
            set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_EBADF);
            result->r0 = (uint32_t)-1;
        } else result->r0 = (uint32_t)got;
        break;
    }
    case POSIX_LSEEK: {
        uint64_t raw_offset = 0u;
        uint32_t whence = 0u;
        int64_t position;
        if (s3e_aapcs32_arg_u64_even(frame, 2u, &raw_offset) != S3E_AAPCS32_OK ||
            s3e_aapcs32_arg_word(frame, 4u, &whence) != S3E_AAPCS32_OK) {
            set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_EFAULT);
            result->r0 = (uint32_t)-1;
            break;
        }
        position = re_guest_vfs_posix_lseek(p->filesystem, (int32_t)a,
                                             (int64_t)raw_offset, (int)whence);
        if (position < 0) {
            set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_EINVAL);
            result->r0 = (uint32_t)-1;
        } else result->r0 = (uint32_t)position;
        break;
    }
    case POSIX_CLOSE:
        if (re_guest_vfs_posix_close(p->filesystem, (int32_t)a) != 0) {
            set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_EBADF);
            result->r0 = (uint32_t)-1;
        }
        break;
    case POSIX_ACCESS: {
        char path[RE_GUEST_VFS_POSIX_IMPORT_MAX_PATH];
        if (!guest_string(bus, a, path)) {
            set_errno(p, bus, RE_GUEST_VFS_POSIX_ERRNO_EFAULT);
            result->r0 = (uint32_t)-1;
        } else if (re_guest_vfs_posix_access(p->filesystem, path, (int)b) != 0) {
            set_errno(p, bus, b & (RE_GUEST_VFS_POSIX_W_OK | RE_GUEST_VFS_POSIX_X_OK)
                ? RE_GUEST_VFS_POSIX_ERRNO_EACCES
                : RE_GUEST_VFS_POSIX_ERRNO_ENOENT);
            result->r0 = (uint32_t)-1;
        }
        break;
    }
    default:
        return RE_IMPORT_E_HANDLER;
    }
    return RE_IMPORT_OK;
}

static int find_symbol(const REGUESTELF32LINKSYMBOL *symbols, size_t count,
                       const char *name, size_t *found) {
    size_t match = count;
    for (size_t i = 0; i < count; ++i) {
        const REGUESTELF32LINKSYMBOL *s = &symbols[i];
        if (!memchr(s->name, '\0', sizeof(s->name))) return 0;
        if (strcmp(s->name, name) != 0 || s->section_index != 0u ||
            (s->binding != RE_GUEST_STB_GLOBAL && s->binding != RE_GUEST_STB_WEAK) ||
            (s->visibility != RE_GUEST_STV_DEFAULT && s->visibility != RE_GUEST_STV_PROTECTED) ||
            (s->type != 0u && s->type != 2u)) continue;
        if (match != count) return 0;
        match = i;
    }
    if (match == count) return 0;
    *found = match;
    return 1;
}

REGUESTVFSPOSIXImportStatus re_guest_vfs_posix_import_provider_init(
    REGUESTVFSPOSIXImportProvider *provider, REGUESTVFSPOSIX *filesystem,
    uint32_t guest_errno_address) {
    if (!provider || !filesystem || !filesystem->initialized)
        return RE_GUEST_VFS_POSIX_IMPORT_E_ARGUMENT;
    memset(provider, 0, sizeof(*provider));
    provider->filesystem = filesystem;
    provider->guest_errno_address = guest_errno_address;
    provider->initialized = 1u;
    for (size_t i = 0; i < RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT; ++i) {
        provider->contexts[i].kind = (uint32_t)i;
        provider->contexts[i].provider = provider;
    }
    return RE_GUEST_VFS_POSIX_IMPORT_OK;
}

REGUESTVFSPOSIXImportStatus re_guest_vfs_posix_import_provider_bind_elf_symbols(
    REGUESTVFSPOSIXImportProvider *provider, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity, size_t *export_count,
    size_t *failed_symbol_index) {
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT];
    size_t indices[RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT];
    uint32_t pointers[RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT];
    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!provider || !provider->initialized || !imports || !symbols ||
        !exports || !export_count) return RE_GUEST_VFS_POSIX_IMPORT_E_ARGUMENT;
    if (export_capacity < RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT ||
        symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS - RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT)
        return RE_GUEST_VFS_POSIX_IMPORT_E_CAPACITY;
    for (size_t i = 0; i < RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT; ++i) {
        if (!find_symbol(symbols, symbol_count, k_names[i], &indices[i])) {
            if (failed_symbol_index) *failed_symbol_index = symbol_count;
            return RE_GUEST_VFS_POSIX_IMPORT_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j)
            if (indices[i] == indices[j]) return RE_GUEST_VFS_POSIX_IMPORT_E_SYMBOL;
    }
    backup = *imports;
    for (size_t i = 0; i < RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT; ++i) {
        if (re_guest_import_bind_mapped(imports, (uint32_t)indices[i],
                posix_handler, &provider->contexts[i], &pointers[i]) != RE_IMPORT_OK) {
            *imports = backup;
            return RE_GUEST_VFS_POSIX_IMPORT_E_BIND;
        }
        pending[i] = (REGUESTSYMBOL){ k_names[i], pointers[i],
            RE_GUEST_STB_GLOBAL, RE_GUEST_STV_DEFAULT, 1u, NULL, "libc.so" };
    }
    memcpy(exports, pending, sizeof(pending));
    *export_count = RE_GUEST_VFS_POSIX_IMPORT_SYMBOL_COUNT;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_VFS_POSIX_IMPORT_OK;
}
