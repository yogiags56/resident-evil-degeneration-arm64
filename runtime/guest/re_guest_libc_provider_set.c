#include "re_guest_libc_provider_set.h"

#include <string.h>

REGUESTLIBCProviderSetStatus re_guest_libc_provider_set_bind_elf_symbols(
    const REGUESTLIBCProviderSet *set, REGUESTIMPORTTable *imports,
    const REGUESTELF32LINKSYMBOL *symbols, size_t symbol_count,
    REGUESTSYMBOL *exports, size_t export_capacity,
    size_t *export_count, size_t *failed_symbol_index) {
    REGUESTIMPORTTable backup;
    REGUESTSYMBOL pending[RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT];
    size_t count = 0u, failed = symbol_count, added = 0u;
    REGUESTLIBCProviderSetStatus status = RE_GUEST_LIBC_PROVIDER_SET_OK;

    if (export_count) *export_count = 0u;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    if (!set || !set->alloc || !set->string || !set->format || !set->stdio ||
        !imports || !symbols || !exports || !export_count)
        return RE_GUEST_LIBC_PROVIDER_SET_E_ARGUMENT;
    if (export_capacity < RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT ||
        symbol_count > RE_GUEST_ELF32_LINK_MAX_SYMBOLS ||
        imports->count > RE_IMPORT_MAX_BINDINGS -
            RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT)
        return RE_GUEST_LIBC_PROVIDER_SET_E_CAPACITY;

    backup = *imports;
    if (re_guest_libc_alloc_provider_bind_elf_symbols(set->alloc, imports,
            symbols, symbol_count, pending, RE_GUEST_LIBC_ALLOC_SYMBOL_COUNT,
            &added, &failed) != RE_GUEST_LIBC_ALLOC_OK) {
        status = RE_GUEST_LIBC_PROVIDER_SET_E_ALLOC;
        goto rollback;
    }
    count += added;
    if (re_guest_libc_string_provider_bind_elf_symbols(set->string, imports,
            symbols, symbol_count, pending + count,
            RE_GUEST_LIBC_STRING_SYMBOL_COUNT, &added, &failed) !=
            RE_GUEST_LIBC_STRING_OK) {
        status = RE_GUEST_LIBC_PROVIDER_SET_E_STRING;
        goto rollback;
    }
    count += added;
    if (re_guest_libc_format_provider_bind_elf_symbols(set->format, imports,
            symbols, symbol_count, pending + count,
            RE_GUEST_LIBC_FORMAT_SYMBOL_COUNT, &added, &failed) !=
            RE_GUEST_LIBC_FORMAT_OK) {
        status = RE_GUEST_LIBC_PROVIDER_SET_E_FORMAT;
        goto rollback;
    }
    count += added;
    if (re_guest_libc_stdio_provider_bind_elf_symbols(set->stdio, imports,
            symbols, symbol_count, pending + count,
            RE_GUEST_LIBC_STDIO_SYMBOL_COUNT, &added, &failed) !=
            RE_GUEST_LIBC_STDIO_OK) {
        status = RE_GUEST_LIBC_PROVIDER_SET_E_STDIO;
        goto rollback;
    }
    count += added;

    if (count != RE_GUEST_LIBC_PROVIDER_SET_SYMBOL_COUNT) {
        status = RE_GUEST_LIBC_PROVIDER_SET_E_CAPACITY;
        goto rollback;
    }
    memcpy(exports, pending, count * sizeof(pending[0]));
    *export_count = count;
    if (failed_symbol_index) *failed_symbol_index = symbol_count;
    return RE_GUEST_LIBC_PROVIDER_SET_OK;

rollback:
    *imports = backup;
    if (failed_symbol_index) *failed_symbol_index = failed;
    return status;
}
