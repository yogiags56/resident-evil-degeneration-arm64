#ifndef RE_GUEST_HEAP_H
#define RE_GUEST_HEAP_H

#include <stddef.h>
#include <stdint.h>

#include "re_guest_memory_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_HEAP_MAX_ALLOCS 512u
#define RE_GUEST_HEAP_DEFAULT_ALIGNMENT 8u
#define RE_GUEST_HEAP_PAGE_ALIGNMENT 4096u

typedef struct {
    uint32_t guest_address;
    uint32_t capacity;
    uint32_t requested_size;
    uint8_t active;
} REGUESTHEAPAllocation;

typedef struct {
    uint32_t heap_base;
    uint32_t heap_size;
    uint8_t busy;
    REGUESTHEAPAllocation allocations[RE_GUEST_HEAP_MAX_ALLOCS];
} REGUESTHEAP;

typedef enum {
    RE_GUEST_HEAP_OK = 0,
    RE_GUEST_HEAP_E_ARGUMENT = -1020,
    RE_GUEST_HEAP_E_MEMORY = -1021
} REGUESTHEAPStatus;

REGUESTHEAPStatus re_guest_heap_init(REGUESTHEAP *heap,
                                     uint32_t heap_base,
                                     uint32_t heap_size);
uint32_t re_guest_heap_alloc(REGUESTHEAP *heap, const REGUESTMEMBUS *bus,
                             uint32_t requested_size, uint32_t alignment);
/* Null and unknown pointers are harmless no-ops; only exact returned
 * guest addresses release allocation records.
 */
void re_guest_heap_free(REGUESTHEAP *heap, uint32_t guest_address);
uint32_t re_guest_heap_realloc(REGUESTHEAP *heap, const REGUESTMEMBUS *bus,
                               uint32_t guest_address,
                               uint32_t requested_size);
void re_guest_heap_shutdown(REGUESTHEAP *heap);

#ifdef __cplusplus
}
#endif
#endif
