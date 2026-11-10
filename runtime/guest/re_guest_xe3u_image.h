#ifndef RE_GUEST_XE3U_IMAGE_H
#define RE_GUEST_XE3U_IMAGE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RE_GUEST_XE3U_HEADER_SIZE 0x48u
#define RE_GUEST_XE3U_MAX_FIXUP_SECTIONS 16u
#define RE_GUEST_XE3U_SYMBOL_NAME_CAP 128u
typedef struct {
    uint32_t version;
    uint16_t flags,architecture;
    uint32_t fixup_offset,fixup_size;
    uint32_t code_offset,code_file_size,code_memory_size;
    uint32_t signature_offset,signature_size;
    uint32_t entry_offset,config_offset,config_size;
    uint32_t base_address,extra_offset,extra_size,extended_header_size;
    uint32_t data_offset;
    uint8_t has_data_offset;
} REGUESTXE3UHEADER;
typedef struct {
    char name[RE_GUEST_XE3U_SYMBOL_NAME_CAP];
} REGUESTXE3USYMBOL;
typedef struct {
    uint32_t offset;
    uint16_t symbol_index;
    uint8_t section_type;
} REGUESTXE3UEXTERNALRELOCATION;
typedef struct {
    uint32_t type,offset,size,count;
} REGUESTXE3UFIXUPSECTION;
typedef struct {
    REGUESTXE3UFIXUPSECTION sections[RE_GUEST_XE3U_MAX_FIXUP_SECTIONS];
    size_t section_count,symbol_count,internal_relocation_count;
    size_t external_relocation_count;
} REGUESTXE3UFIXUPINFO;
typedef enum {
    RE_GUEST_XE3U_OK=0,
    RE_GUEST_XE3U_E_ARGUMENT=-820,
    RE_GUEST_XE3U_E_HEADER=-821,
    RE_GUEST_XE3U_E_RANGE=-822,
    RE_GUEST_XE3U_E_CAPACITY=-823,
    RE_GUEST_XE3U_E_FIXUP=-824,
    RE_GUEST_XE3U_E_SYMBOL=-825,
    RE_GUEST_XE3U_E_DUPLICATE=-826,
    RE_GUEST_XE3U_E_UNSUPPORTED=-827
} REGUESTXE3UStatus;
/* Read-only XE3U header parser. It does not decompress, map, relocate, or run. */
REGUESTXE3UStatus re_guest_xe3u_parse_header(
    const uint8_t *bytes,size_t size,REGUESTXE3UHEADER *header,
    size_t *failed_field);
/* Bounded, caller-buffered fixup decoder. All caller output arrays and info
 * remain unchanged on failure. Unknown subsection types are recorded/skipped.
 */
REGUESTXE3UStatus re_guest_xe3u_parse_fixups(
    const uint8_t *bytes,size_t size,const REGUESTXE3UHEADER *header,
    REGUESTXE3USYMBOL *symbols,size_t symbol_capacity,
    uint32_t *internal_offsets,size_t internal_capacity,
    REGUESTXE3UEXTERNALRELOCATION *external,size_t external_capacity,
    REGUESTXE3UFIXUPINFO *info,size_t *failed_section,size_t *failed_record);
#ifdef __cplusplus
}
#endif
#endif
