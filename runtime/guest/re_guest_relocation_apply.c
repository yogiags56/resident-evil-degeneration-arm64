#include "re_guest_relocation_apply.h"
#include <string.h>

typedef struct {
    uint8_t *destination;
    uint32_t value;
} RE_RELOC_PLAN;

static uint32_t read_u32le(const uint8_t *p) {
    return (uint32_t)p[0] |
        ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) |
        ((uint32_t)p[3] << 24);
}

static void write_u32le(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

REGUESTRELOCApplyStatus re_guest_relocations_apply(
    REGUESTMEMORY *memory,
    const REGUESTRELOCATION *relocations,
    size_t count,
    size_t *failed_index) {
    RE_RELOC_PLAN plans[RE_GUEST_MEMORY_MAX_REGIONS];

    if (failed_index) *failed_index = count;
    if (!memory || (!relocations && count) ||
        count > RE_GUEST_MEMORY_MAX_REGIONS)
        return RE_GUEST_RELOC_APPLY_E_ARGUMENT;

    /* Preflight: validate every relocation and compute every result. */
    for (size_t i = 0; i < count; ++i) {
        const REGUESTRELOCATION *r = &relocations[i];
        uint32_t addend = r->addend;
        uint32_t value;
        void *target = NULL;

        plans[i].destination = NULL;
        plans[i].value = 0;

        if (r->target & 3u) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_RELOC_APPLY_E_ALIGNMENT;
        }
        if (r->has_explicit_addend > 1u || r->symbol_resolved > 1u) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_RELOC_APPLY_E_ARGUMENT;
        }
        if (r->type != RE_ARM_R_ABS32 &&
            r->type != RE_ARM_R_GLOB_DAT &&
            r->type != RE_ARM_R_JUMP_SLOT &&
            r->type != RE_ARM_R_RELATIVE) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_RELOC_APPLY_E_UNSUPPORTED;
        }
        if (r->type == RE_ARM_R_RELATIVE && r->symbol_index != 0u) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_RELOC_APPLY_E_RELATIVE_SYMBOL;
        }
        if ((r->type == RE_ARM_R_ABS32 ||
             r->type == RE_ARM_R_GLOB_DAT ||
             r->type == RE_ARM_R_JUMP_SLOT) && !r->symbol_resolved) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_RELOC_APPLY_E_SYMBOL;
        }
        for (size_t j = 0; j < i; ++j) {
            if (relocations[j].target == r->target) {
                if (failed_index) *failed_index = i;
                return RE_GUEST_RELOC_APPLY_E_DUPLICATE;
            }
        }

        if (!r->has_explicit_addend &&
            (r->type == RE_ARM_R_ABS32 || r->type == RE_ARM_R_RELATIVE)) {
            if (re_guest_memory_resolve(memory, r->target, 4u,
                    RE_GUEST_MEM_READ, &target) != RE_GUEST_MEM_OK) {
                if (failed_index) *failed_index = i;
                return RE_GUEST_RELOC_APPLY_E_TARGET;
            }
            addend = read_u32le((const uint8_t *)target);
        }

        target = NULL;
        if (re_guest_memory_resolve(memory, r->target, 4u,
                RE_GUEST_MEM_WRITE, &target) != RE_GUEST_MEM_OK) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_RELOC_APPLY_E_PERMISSION;
        }

        switch (r->type) {
        case RE_ARM_R_RELATIVE:
            value = r->load_bias + addend;
            break;
        case RE_ARM_R_ABS32:
            value = r->symbol_value + addend;
            break;
        case RE_ARM_R_GLOB_DAT:
        case RE_ARM_R_JUMP_SLOT:
            value = r->symbol_value;
            break;
        default:
            if (failed_index) *failed_index = i;
            return RE_GUEST_RELOC_APPLY_E_UNSUPPORTED;
        }
        plans[i].destination = (uint8_t *)target;
        plans[i].value = value;
    }

    /* Commit: bounded byte stores only; no further lookup can fail. */
    for (size_t i = 0; i < count; ++i)
        write_u32le(plans[i].destination, plans[i].value);

    return RE_GUEST_RELOC_APPLY_OK;
}
