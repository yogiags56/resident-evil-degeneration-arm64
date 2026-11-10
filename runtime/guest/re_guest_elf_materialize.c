#include "re_guest_elf_materialize.h"
#include <string.h>

#define RE_GUEST_ELF_ADDRESS_LIMIT 0x100000000ULL

typedef struct {
    uint8_t *destination;
    uint64_t guest_start;
    uint64_t guest_end;
} RE_MATERIALIZE_PLAN;

REGUESTELFMaterializeStatus re_guest_elf_materialize(
    REGUESTMEMORY *memory,
    const uint8_t *elf_bytes,
    size_t elf_size,
    const REGUESTELFSEGMENT *segments,
    size_t count,
    size_t *failed_index) {
    RE_MATERIALIZE_PLAN plans[RE_GUEST_MEMORY_MAX_REGIONS];

    if (failed_index) *failed_index = count;
    if (!memory || (!elf_bytes && elf_size) || (!segments && count) ||
        count > RE_GUEST_MEMORY_MAX_REGIONS) {
        return RE_GUEST_ELF_MATERIALIZE_E_ARGUMENT;
    }

    /* Preflight: no guest byte is changed until every segment is validated. */
    for (size_t i = 0; i < count; ++i) {
        const REGUESTELFSEGMENT *s = &segments[i];
        uint64_t guest_start = s->guest_address;
        uint64_t guest_end;

        plans[i].destination = NULL;
        plans[i].guest_start = guest_start;
        plans[i].guest_end = guest_start;

        if (s->file_size > s->memory_size) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_ELF_MATERIALIZE_E_SIZE;
        }
        if (s->file_offset > (uint64_t)elf_size ||
            s->file_size > (uint64_t)elf_size - s->file_offset) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_ELF_MATERIALIZE_E_SOURCE_RANGE;
        }
        if (s->memory_size > RE_GUEST_ELF_ADDRESS_LIMIT - guest_start ||
            s->memory_size > (uint64_t)SIZE_MAX) {
            if (failed_index) *failed_index = i;
            return RE_GUEST_ELF_MATERIALIZE_E_GUEST_RANGE;
        }
        guest_end = guest_start + s->memory_size;
        plans[i].guest_end = guest_end;

        if (s->memory_size == 0) continue;

        for (size_t j = 0; j < i; ++j) {
            if (plans[j].guest_start < plans[j].guest_end &&
                guest_start < plans[j].guest_end &&
                plans[j].guest_start < guest_end) {
                if (failed_index) *failed_index = i;
                return RE_GUEST_ELF_MATERIALIZE_E_OVERLAP;
            }
        }

        {
            void *destination = NULL;
            REGUESTMEMStatus status = re_guest_memory_resolve(
                memory, s->guest_address, (size_t)s->memory_size,
                RE_GUEST_MEM_WRITE, &destination);
            if (status != RE_GUEST_MEM_OK) {
                if (failed_index) *failed_index = i;
                return status == RE_GUEST_MEM_E_PERMISSION
                    ? RE_GUEST_ELF_MATERIALIZE_E_PERMISSION
                    : RE_GUEST_ELF_MATERIALIZE_E_GUEST_RANGE;
            }
            plans[i].destination = (uint8_t *)destination;
        }
    }

    /* Commit phase: all operations below are bounded and cannot fail. */
    for (size_t i = 0; i < count; ++i) {
        const REGUESTELFSEGMENT *s = &segments[i];
        if (s->file_size) {
            memcpy(plans[i].destination, elf_bytes + (size_t)s->file_offset,
                (size_t)s->file_size);
        }
        if (s->memory_size > s->file_size) {
            memset(plans[i].destination + (size_t)s->file_size, 0,
                (size_t)(s->memory_size - s->file_size));
        }
    }
    return RE_GUEST_ELF_MATERIALIZE_OK;
}
