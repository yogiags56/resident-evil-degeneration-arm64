#include "re_guest_map_transaction.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    REGUESTMEMORY memory;
    uint8_t original[16] = {0};
    uint8_t first[32] = {0}, second[32] = {0}, third[32] = {0};
    size_t failed = 999;
    REGUESTMEMStatus map_status = RE_GUEST_MEM_OK;
    REGUESTMEMMAPREQUEST requests[3];

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory, 0x1000, sizeof(original), original,
        RE_GUEST_MEM_READ) == RE_GUEST_MEM_OK);

    requests[0] = (REGUESTMEMMAPREQUEST){0x2000, sizeof(first), first,
        RE_GUEST_MEM_READ, RE_GUEST_MEM_TYPE_NORMAL};
    requests[1] = (REGUESTMEMMAPREQUEST){0x3000, sizeof(second), second,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE, RE_GUEST_MEM_TYPE_NORMAL};
    assert(re_guest_memory_map_transaction(&memory, requests, 2, &failed,
        &map_status) == RE_GUEST_MAP_TX_OK);
    assert(failed == 2 && map_status == RE_GUEST_MEM_OK);
    assert(re_guest_memory_region_count(&memory) == 3);

    /* Existing mapping overlap fails; the earlier new mapping is rolled back. */
    requests[0] = (REGUESTMEMMAPREQUEST){0x4000, sizeof(first), first,
        RE_GUEST_MEM_READ, RE_GUEST_MEM_TYPE_NORMAL};
    requests[1] = (REGUESTMEMMAPREQUEST){0x1000, sizeof(original), second,
        RE_GUEST_MEM_READ, RE_GUEST_MEM_TYPE_NORMAL};
    assert(re_guest_memory_map_transaction(&memory, requests, 2, &failed,
        &map_status) == RE_GUEST_MAP_TX_E_MAP);
    assert(failed == 1 && map_status == RE_GUEST_MEM_E_OVERLAP);
    assert(re_guest_memory_region_count(&memory) == 3);
    assert(re_guest_memory_resolve(&memory, 0x4000, 1, RE_GUEST_MEM_READ,
        (void **)&requests[2].host_buffer) == RE_GUEST_MEM_E_NOT_FOUND);

    /* Duplicate guest ranges within one transaction also roll back cleanly. */
    requests[0] = (REGUESTMEMMAPREQUEST){0x5000, sizeof(first), first,
        RE_GUEST_MEM_READ, RE_GUEST_MEM_TYPE_NORMAL};
    requests[1] = (REGUESTMEMMAPREQUEST){0x5000, sizeof(second), second,
        RE_GUEST_MEM_READ, RE_GUEST_MEM_TYPE_NORMAL};
    assert(re_guest_memory_map_transaction(&memory, requests, 2, &failed,
        &map_status) == RE_GUEST_MAP_TX_E_MAP);
    assert(failed == 1 && map_status == RE_GUEST_MEM_E_OVERLAP);
    assert(re_guest_memory_region_count(&memory) == 3);

    /* Invalid later request does not leave an earlier new region mapped. */
    requests[0] = (REGUESTMEMMAPREQUEST){0x6000, sizeof(first), first,
        RE_GUEST_MEM_READ, RE_GUEST_MEM_TYPE_NORMAL};
    requests[1] = (REGUESTMEMMAPREQUEST){0x7000, sizeof(third), third,
        0, RE_GUEST_MEM_TYPE_NORMAL};
    assert(re_guest_memory_map_transaction(&memory, requests, 2, &failed,
        &map_status) == RE_GUEST_MAP_TX_E_MAP);
    assert(failed == 1 && map_status == RE_GUEST_MEM_E_ARGUMENT);
    assert(re_guest_memory_region_count(&memory) == 3);

    /* The pre-existing region survives every rollback. */
    {
        void *resolved = NULL;
        assert(re_guest_memory_resolve(&memory, 0x1000, 1, RE_GUEST_MEM_READ,
            &resolved) == RE_GUEST_MEM_OK);
        assert(resolved == original);
    }

    assert(re_guest_memory_map_transaction(&memory, NULL, 0, &failed,
        &map_status) == RE_GUEST_MAP_TX_OK);
    assert(failed == 0);
    assert(re_guest_memory_map_transaction(&memory, NULL, 1, &failed,
        &map_status) == RE_GUEST_MAP_TX_E_ARGUMENT);
    puts("Guest memory map transaction tests: PASS");
    return 0;
}
