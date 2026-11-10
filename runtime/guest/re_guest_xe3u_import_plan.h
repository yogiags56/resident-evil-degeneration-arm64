#ifndef RE_GUEST_XE3U_IMPORT_PLAN_H
#define RE_GUEST_XE3U_IMPORT_PLAN_H
#include "re_guest_xe3u_image.h"
#include "re_guest_imports.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t cell_offset;
    uint32_t original_value;
    uint32_t trampoline_value;
    uint16_t symbol_index;
    uint8_t section_type;
    uint8_t is_arm_stub;
} REGUESTXE3UEXTERNALPATCH;
typedef enum {
    RE_GUEST_XE3U_IMPORT_PLAN_OK=0,
    RE_GUEST_XE3U_IMPORT_PLAN_E_ARGUMENT=-860,
    RE_GUEST_XE3U_IMPORT_PLAN_E_CAPACITY=-861,
    RE_GUEST_XE3U_IMPORT_PLAN_E_DUPLICATE=-862,
    RE_GUEST_XE3U_IMPORT_PLAN_E_RANGE=-863,
    RE_GUEST_XE3U_IMPORT_PLAN_E_FORMAT=-864,
    RE_GUEST_XE3U_IMPORT_PLAN_E_UNBOUND=-865,
    RE_GUEST_XE3U_IMPORT_PLAN_E_MEMORY=-866,
    RE_GUEST_XE3U_IMPORT_PLAN_E_STALE=-867,
    RE_GUEST_XE3U_IMPORT_PLAN_E_ROLLBACK=-868
} REGUESTXE3UIMPORTPLANStatus;
/* Build a symbol-specific Thumb-trampoline plan for all external XE3U cells.
 * Original type-4 cells must contain either base+0x70 (verified ARM import
 * stub) or zero (function-pointer data cell). Unknown imports may optionally
 * be pointed at the runtime unresolved trap. No image bytes are modified.
 */
REGUESTXE3UIMPORTPLANStatus re_guest_xe3u_prepare_external_import_plan(
    const uint8_t *original_code,size_t code_size,const REGUESTXE3UHEADER *header,
    size_t symbol_count,const REGUESTXE3UEXTERNALRELOCATION *external,
    size_t external_count,const REGUESTIMPORTTable *imports,uint32_t runtime_base,
    uint32_t unresolved_placeholder_pc,int allow_unbound,
    uint32_t *scratch,size_t scratch_capacity,
    REGUESTXE3UEXTERNALPATCH *output,size_t output_capacity,
    size_t *unbound_count,size_t *failed_index);
/* Apply a fully preflighted plan to mapped writable guest image memory.
 * All old values are checked before the first write; a failed write triggers
 * rollback of earlier cells. The code image must still contain original values.
 */
REGUESTXE3UIMPORTPLANStatus re_guest_xe3u_apply_external_import_plan(
    REGUESTMEMBUS *bus,const REGUESTXE3UHEADER *header,uint32_t runtime_base,
    const REGUESTXE3UEXTERNALPATCH *plan,size_t count,size_t *failed_index);
#ifdef __cplusplus
}
#endif
#endif
