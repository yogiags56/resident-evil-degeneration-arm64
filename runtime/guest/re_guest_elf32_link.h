#ifndef RE_GUEST_ELF32_LINK_H
#define RE_GUEST_ELF32_LINK_H

#include "re_guest_elf32_dynamic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_ELF32_LINK_MAX_SYMBOLS 4096u
#define RE_GUEST_ELF32_LINK_MAX_RELOCATIONS 4096u
#define RE_GUEST_ELF32_LINK_SYMBOL_NAME_CAP 128u

typedef struct {
    uint32_t name_offset, value, size;
    uint16_t section_index;
    uint8_t binding, type, visibility;
    char name[RE_GUEST_ELF32_LINK_SYMBOL_NAME_CAP];
} REGUESTELF32LINKSYMBOL;

typedef struct {
    uint32_t offset, symbol_index, type;
    int32_t addend;
    uint8_t has_addend, is_plt;
} REGUESTELF32LINKRELOCATION;

typedef struct {
    uint32_t symbol_table_address, symbol_entry_size, symbol_count;
    uint32_t sysv_hash_address, string_table_address, string_table_size;
    uint32_t relocation_count, plt_relocation_count;
    uint8_t has_symbols, has_sysv_hash, has_rel, has_rela, has_plt;
} REGUESTELF32LINKINFO;

typedef enum {
    RE_GUEST_ELF32_LINK_OK = 0,
    RE_GUEST_ELF32_LINK_E_ARGUMENT = -640,
    RE_GUEST_ELF32_LINK_E_TAG = -641,
    RE_GUEST_ELF32_LINK_E_RANGE = -642,
    RE_GUEST_ELF32_LINK_E_FORMAT = -643,
    RE_GUEST_ELF32_LINK_E_CAPACITY = -644,
    RE_GUEST_ELF32_LINK_E_STRING = -645,
    RE_GUEST_ELF32_LINK_E_SYMBOL = -646,
    RE_GUEST_ELF32_LINK_E_RELOCATION = -647,
    RE_GUEST_ELF32_LINK_E_UNSUPPORTED = -648
} REGUESTELF32LINKStatus;

/*
 * Read-only bounded decoder for ELF32 ARM dynamic symbols and REL/RELA
 * relocation records. It uses DT_HASH.nchain for the symbol bound and
 * PT_LOAD file-backed ranges for every virtual-to-file translation.
 *
 * No dependency search, symbol resolution, load-bias adjustment, memory
 * mutation, relocation application, constructor, or guest execution occurs.
 * GNU-hash-only images and relocation encodings other than REL/RELA are
 * rejected explicitly rather than guessed.
 */
REGUESTELF32LINKStatus re_guest_elf32_parse_link_metadata(
    const uint8_t *bytes, size_t size,
    const REGUESTELF32IMAGE *image,
    const REGUESTELF32DYNAMIC *dynamic,
    REGUESTELF32LINKINFO *info,
    REGUESTELF32LINKSYMBOL *symbols, size_t symbol_capacity,
    REGUESTELF32LINKRELOCATION *relocations, size_t relocation_capacity,
    size_t *failed_index);

#ifdef __cplusplus
}
#endif
#endif
