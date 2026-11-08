#ifndef RE_GUEST_MEMORY_H
#define RE_GUEST_MEMORY_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RE_GUEST_MEMORY_MAX_REGIONS 32u

enum {
    RE_GUEST_MEM_READ  = 1u << 0,
    RE_GUEST_MEM_WRITE = 1u << 1,
    RE_GUEST_MEM_EXEC  = 1u << 2
};

typedef enum {
    RE_GUEST_MEM_OK = 0,
    RE_GUEST_MEM_E_ARGUMENT = -1,
    RE_GUEST_MEM_E_CAPACITY = -2,
    RE_GUEST_MEM_E_OVERLAP = -3,
    RE_GUEST_MEM_E_RANGE = -4,
    RE_GUEST_MEM_E_PERMISSION = -5,
    RE_GUEST_MEM_E_NOT_FOUND = -6
} REGUESTMEMStatus;

typedef struct {
    uint32_t base;
    uint64_t size;
    uint8_t *host;
    uint32_t permissions;
    uint8_t active;
} REGUESTMEMRegion;

typedef struct {
    REGUESTMEMRegion regions[RE_GUEST_MEMORY_MAX_REGIONS];
    size_t count;
} REGUESTMEMORY;

/* The manager borrows host buffers; callers own their allocation and lifetime. */
void re_guest_memory_init(REGUESTMEMORY *memory);
REGUESTMEMStatus re_guest_memory_map(REGUESTMEMORY *memory, uint32_t guest_base,
    uint64_t size, void *host_buffer, uint32_t permissions);
REGUESTMEMStatus re_guest_memory_unmap(REGUESTMEMORY *memory, uint32_t guest_base);
REGUESTMEMStatus re_guest_memory_resolve(const REGUESTMEMORY *memory,
    uint32_t guest_address, size_t length, uint32_t required_permissions,
    void **host_pointer);
size_t re_guest_memory_region_count(const REGUESTMEMORY *memory);

#ifdef __cplusplus
}
#endif
#endif
