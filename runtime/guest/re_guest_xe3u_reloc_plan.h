#ifndef RE_GUEST_XE3U_RELOC_PLAN_H
#define RE_GUEST_XE3U_RELOC_PLAN_H
#include "re_guest_xe3u_image.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t patch_offset;
    uint32_t original_value;
    uint32_t relocated_value;
    uint8_t thumb_state;
} REGUESTXE3URELOCATION;
typedef enum {
    RE_GUEST_XE3U_RELOC_PLAN_OK=0,
    RE_GUEST_XE3U_RELOC_PLAN_E_ARGUMENT=-840,
    RE_GUEST_XE3U_RELOC_PLAN_E_CAPACITY=-841,
    RE_GUEST_XE3U_RELOC_PLAN_E_DUPLICATE=-842,
    RE_GUEST_XE3U_RELOC_PLAN_E_PATCH_RANGE=-843,
    RE_GUEST_XE3U_RELOC_PLAN_E_TARGET_RANGE=-844,
    RE_GUEST_XE3U_RELOC_PLAN_E_ADDRESS_OVERFLOW=-845,
    RE_GUEST_XE3U_RELOC_PLAN_E_ALIGNMENT=-846
} REGUESTXE3URELOCPLANStatus;
/*
 * Candidate-only planner for XE3U type-1 internal pointer relocations.
 * It validates every patch and target, then publishes a complete plan.
 * It NEVER writes code bytes or guest memory. The caller supplies a scratch
 * hash table with capacity >= 2 * relocation_count; scratch is clobbered.
 * Output remains unchanged on every failure. runtime_base must be 4 KiB
 * aligned. External type-2/3/4 records are deliberately excluded.
 *
 * Candidate operation: relocated_value = original_value +
 * (runtime_base - header->base_address), preserving bit 0 (ARM/Thumb state).
 * This operation is supported by original-image evidence but is not yet
 * connected to the production loader or an image-memory mutation path.
 */
REGUESTXE3URELOCPLANStatus re_guest_xe3u_prepare_internal_relocations(
    const uint8_t *code,size_t code_size,const REGUESTXE3UHEADER *header,
    const uint32_t *offsets,size_t count,uint32_t runtime_base,
    uint32_t *scratch,size_t scratch_capacity,
    REGUESTXE3URELOCATION *output,size_t output_capacity,
    size_t *failed_index);
#ifdef __cplusplus
}
#endif
#endif
