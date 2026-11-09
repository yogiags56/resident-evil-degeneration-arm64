#ifndef RE_GUEST_MAP_TRANSACTION_H
#define RE_GUEST_MAP_TRANSACTION_H

#include "re_guest_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t guest_base;
    uint64_t size;
    void *host_buffer;
    uint32_t permissions;
    REGUESTMEMType memory_type;
} REGUESTMEMMAPREQUEST;

typedef enum {
    RE_GUEST_MAP_TX_OK = 0,
    RE_GUEST_MAP_TX_E_ARGUMENT = -100,
    RE_GUEST_MAP_TX_E_MAP = -101,
    RE_GUEST_MAP_TX_E_ROLLBACK = -102
} REGUESTMEMMAPTXStatus;

/*
 * Map every request or remove every mapping created by this call on failure.
 * Existing mappings are never removed. Buffers remain borrowed from caller.
 * failed_index receives the failing request index, or count on success.
 */
REGUESTMEMMAPTXStatus re_guest_memory_map_transaction(
    REGUESTMEMORY *memory,
    const REGUESTMEMMAPREQUEST *requests,
    size_t count,
    size_t *failed_index,
    REGUESTMEMStatus *map_status);

#ifdef __cplusplus
}
#endif
#endif
