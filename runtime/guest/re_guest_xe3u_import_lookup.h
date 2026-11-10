#ifndef RE_GUEST_XE3U_IMPORT_LOOKUP_H
#define RE_GUEST_XE3U_IMPORT_LOOKUP_H
#include "re_guest_xe3u_image.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t cell_offset;
    uint32_t stub_offset;
    uint16_t symbol_index;
    uint8_t section_type;
} REGUESTXE3UIMPORTCELL;
typedef enum {
    RE_GUEST_XE3U_IMPORT_LOOKUP_OK=0,
    RE_GUEST_XE3U_IMPORT_LOOKUP_E_ARGUMENT=-850,
    RE_GUEST_XE3U_IMPORT_LOOKUP_E_RANGE=-851,
    RE_GUEST_XE3U_IMPORT_LOOKUP_E_NOT_FOUND=-852,
    RE_GUEST_XE3U_IMPORT_LOOKUP_E_DUPLICATE=-853,
    RE_GUEST_XE3U_IMPORT_LOOKUP_E_UNSUPPORTED=-854,
    RE_GUEST_XE3U_IMPORT_LOOKUP_E_STUB=-855,
    RE_GUEST_XE3U_IMPORT_LOOKUP_E_SYMBOL=-856
} REGUESTXE3UIMPORTLOOKUPStatus;
/*
 * Resolve the identity of an original type-4 import thunk from the guest r12
 * value left by LDR pc,[r12,#imm]!. code must be the immutable original code
 * blob. This is a read-only cell->symbol lookup; it does not bind or call APIs.
 */
REGUESTXE3UIMPORTLOOKUPStatus re_guest_xe3u_lookup_import_cell(
    const uint8_t *code,size_t code_size,const REGUESTXE3UHEADER *header,
    const REGUESTXE3USYMBOL *symbols,size_t symbol_count,
    const REGUESTXE3UEXTERNALRELOCATION *external,size_t external_count,
    uint32_t runtime_base,uint32_t r12_value,
    REGUESTXE3UIMPORTCELL *result,size_t *failed_index);
#ifdef __cplusplus
}
#endif
#endif
