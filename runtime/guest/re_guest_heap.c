#include "re_guest_heap.h"

#include <string.h>

static int is_power_of_two(uint32_t value) {
    return value && (value & (value - 1u)) == 0u;
}

static uint64_t align_up_u64(uint64_t value, uint32_t alignment) {
    return (value + (uint64_t)alignment - 1u) &
           ~((uint64_t)alignment - 1u);
}

static int heap_mapping_valid(const REGUESTHEAP *heap,
                              const REGUESTMEMBUS *bus) {
    void *host = NULL;
    if (!heap || !bus || !bus->memory) return 0;
    return re_guest_memory_resolve(bus->memory, heap->heap_base,
        heap->heap_size, RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE,
        &host) == RE_GUEST_MEM_OK;
}

static int find_free_record(REGUESTHEAP *heap, size_t *index) {
    for (size_t i = 0; i < RE_GUEST_HEAP_MAX_ALLOCS; ++i) {
        if (!heap->allocations[i].active) {
            *index = i;
            return 1;
        }
    }
    return 0;
}

static REGUESTHEAPAllocation *find_record(REGUESTHEAP *heap,
                                           uint32_t address) {
    if (!heap || !address) return NULL;
    for (size_t i = 0; i < RE_GUEST_HEAP_MAX_ALLOCS; ++i) {
        REGUESTHEAPAllocation *record = &heap->allocations[i];
        if (record->active && record->guest_address == address)
            return record;
    }
    return NULL;
}

static uint32_t alloc_unlocked(REGUESTHEAP *heap, const REGUESTMEMBUS *bus,
                               uint32_t requested_size, uint32_t alignment) {
    uint64_t aligned_size, heap_end, candidate;
    size_t free_record = 0u;

    if (!heap_mapping_valid(heap, bus) ||
        !is_power_of_two(alignment) ||
        alignment < RE_GUEST_HEAP_DEFAULT_ALIGNMENT ||
        alignment > 65536u ||
        !find_free_record(heap, &free_record))
        return 0u;

    if (!requested_size) requested_size = 1u;
    aligned_size = align_up_u64(requested_size,
                                RE_GUEST_HEAP_DEFAULT_ALIGNMENT);
    heap_end = (uint64_t)heap->heap_base + heap->heap_size;
    if (aligned_size > UINT32_MAX ||
        heap_end > (uint64_t)UINT32_MAX + 1u)
        return 0u;

    candidate = align_up_u64(heap->heap_base, alignment);
    for (;;) {
        uint64_t candidate_end = candidate + aligned_size;
        uint64_t conflict_end = 0u;
        for (size_t i = 0; i < RE_GUEST_HEAP_MAX_ALLOCS; ++i) {
            const REGUESTHEAPAllocation *block = &heap->allocations[i];
            uint64_t block_start, block_end;
            if (!block->active) continue;
            block_start = block->guest_address;
            block_end = block_start + block->capacity;
            /* Include blocks that begin before the aligned candidate but
             * still overlap it (notably valloc on a non-page-aligned arena).
             */
            if (block_start < candidate_end && block_end > candidate &&
                block_end > conflict_end)
                conflict_end = block_end;
        }
        if (!conflict_end) {
            if (candidate <= UINT32_MAX && candidate_end <= heap_end)
                break;
            return 0u;
        }
        candidate = align_up_u64(conflict_end, alignment);
        if (candidate > heap_end) return 0u;
    }

    if (candidate > UINT32_MAX || candidate + aligned_size > heap_end)
        return 0u;
    heap->allocations[free_record].guest_address = (uint32_t)candidate;
    heap->allocations[free_record].capacity = (uint32_t)aligned_size;
    heap->allocations[free_record].requested_size = requested_size;
    heap->allocations[free_record].active = 1u;
    return (uint32_t)candidate;
}

static void free_unlocked(REGUESTHEAP *heap, uint32_t guest_address) {
    REGUESTHEAPAllocation *record = find_record(heap, guest_address);
    if (record) memset(record, 0, sizeof(*record));
}

REGUESTHEAPStatus re_guest_heap_init(REGUESTHEAP *heap,
                                     uint32_t heap_base,
                                     uint32_t heap_size) {
    uint64_t heap_end = (uint64_t)heap_base + heap_size;
    if (!heap || !heap_base || !heap_size ||
        (heap_base & (RE_GUEST_HEAP_DEFAULT_ALIGNMENT - 1u)) ||
        (heap_size & (RE_GUEST_HEAP_DEFAULT_ALIGNMENT - 1u)) ||
        heap_end > (uint64_t)UINT32_MAX + 1u)
        return RE_GUEST_HEAP_E_ARGUMENT;
    memset(heap, 0, sizeof(*heap));
    heap->heap_base = heap_base;
    heap->heap_size = heap_size;
    return RE_GUEST_HEAP_OK;
}

uint32_t re_guest_heap_alloc(REGUESTHEAP *heap, const REGUESTMEMBUS *bus,
                             uint32_t requested_size, uint32_t alignment) {
    uint32_t address;
    if (!heap || !bus || heap->busy) return 0u;
    heap->busy = 1u;
    address = alloc_unlocked(heap, bus, requested_size, alignment);
    heap->busy = 0u;
    return address;
}

void re_guest_heap_free(REGUESTHEAP *heap, uint32_t guest_address) {
    if (!heap || !guest_address || heap->busy) return;
    heap->busy = 1u;
    free_unlocked(heap, guest_address);
    heap->busy = 0u;
}

uint32_t re_guest_heap_realloc(REGUESTHEAP *heap, const REGUESTMEMBUS *bus,
                               uint32_t guest_address,
                               uint32_t requested_size) {
    REGUESTHEAPAllocation *record;
    uint32_t old_requested, old_capacity, replacement;
    size_t copy_size;
    void *old_host = NULL, *new_host = NULL;

    if (!heap || !bus || heap->busy) return 0u;
    if (!guest_address)
        return re_guest_heap_alloc(heap, bus, requested_size,
                                   RE_GUEST_HEAP_DEFAULT_ALIGNMENT);
    if (!requested_size) {
        re_guest_heap_free(heap, guest_address);
        return 0u;
    }

    heap->busy = 1u;
    record = find_record(heap, guest_address);
    if (!record || !heap_mapping_valid(heap, bus)) {
        heap->busy = 0u;
        return 0u;
    }
    old_requested = record->requested_size;
    old_capacity = record->capacity;
    if (requested_size <= old_capacity) {
        record->requested_size = requested_size;
        heap->busy = 0u;
        return guest_address;
    }

    replacement = alloc_unlocked(heap, bus, requested_size,
                                 RE_GUEST_HEAP_DEFAULT_ALIGNMENT);
    if (!replacement) {
        heap->busy = 0u;
        return 0u;
    }
    copy_size = old_requested < requested_size ? old_requested : requested_size;
    if (re_guest_memory_resolve(bus->memory, guest_address, copy_size,
            RE_GUEST_MEM_READ, &old_host) != RE_GUEST_MEM_OK ||
        re_guest_memory_resolve(bus->memory, replacement, copy_size,
            RE_GUEST_MEM_WRITE, &new_host) != RE_GUEST_MEM_OK) {
        free_unlocked(heap, replacement);
        heap->busy = 0u;
        return 0u;
    }
    memcpy(new_host, old_host, copy_size);
    free_unlocked(heap, guest_address);
    heap->busy = 0u;
    return replacement;
}

void re_guest_heap_shutdown(REGUESTHEAP *heap) {
    if (heap) memset(heap, 0, sizeof(*heap));
}
