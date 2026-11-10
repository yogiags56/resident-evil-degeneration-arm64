#include "re_guest_permission_finalize.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    REGUESTMEMORY memory;
    uint8_t code[64] = {0}, data[64] = {0}, ro[64] = {0};
    size_t failed = 999;
    void *pointer = NULL;
    REGUESTMEMPERMISSIONREQUEST requests[3];

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map(&memory, 0x1000, sizeof(code), code,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory, 0x2000, sizeof(data), data,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_map(&memory, 0x3000, sizeof(ro), ro,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE) == RE_GUEST_MEM_OK);

    requests[0] = (REGUESTMEMPERMISSIONREQUEST){0x1000, sizeof(code),
        RE_GUEST_MEM_READ | RE_GUEST_MEM_EXEC};
    requests[1] = (REGUESTMEMPERMISSIONREQUEST){0x2000, sizeof(data),
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE};
    requests[2] = (REGUESTMEMPERMISSIONREQUEST){0x3000, sizeof(ro),
        RE_GUEST_MEM_READ};
    assert(re_guest_memory_finalize_permissions(&memory, requests, 3,
        &failed) == RE_GUEST_PERMISSION_FINALIZE_OK);
    assert(failed == 3);
    assert(re_guest_memory_resolve(&memory, 0x1000, 1, RE_GUEST_MEM_EXEC,
        &pointer) == RE_GUEST_MEM_OK && pointer == code);
    assert(re_guest_memory_resolve(&memory, 0x1000, 1, RE_GUEST_MEM_WRITE,
        &pointer) == RE_GUEST_MEM_E_PERMISSION && pointer == NULL);
    assert(re_guest_memory_resolve(&memory, 0x2000, 1, RE_GUEST_MEM_WRITE,
        &pointer) == RE_GUEST_MEM_OK && pointer == data);
    assert(re_guest_memory_resolve(&memory, 0x3000, 1, RE_GUEST_MEM_WRITE,
        &pointer) == RE_GUEST_MEM_E_PERMISSION);

    /* W+X request must leave every earlier permission untouched. */
    requests[0].final_permissions = RE_GUEST_MEM_READ;
    requests[1].final_permissions = RE_GUEST_MEM_READ | RE_GUEST_MEM_EXEC;
    requests[2].final_permissions = RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE |
        RE_GUEST_MEM_EXEC;
    assert(re_guest_memory_finalize_permissions(&memory, requests, 3,
        &failed) == RE_GUEST_PERMISSION_FINALIZE_E_WX);
    assert(failed == 2);
    assert(re_guest_memory_resolve(&memory, 0x1000, 1, RE_GUEST_MEM_EXEC,
        &pointer) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_resolve(&memory, 0x2000, 1, RE_GUEST_MEM_WRITE,
        &pointer) == RE_GUEST_MEM_OK);

    requests[2].final_permissions = RE_GUEST_MEM_READ;
    requests[1].guest_base = 0x9000;
    assert(re_guest_memory_finalize_permissions(&memory, requests, 3,
        &failed) == RE_GUEST_PERMISSION_FINALIZE_E_NOT_FOUND);
    assert(failed == 1);
    assert(re_guest_memory_resolve(&memory, 0x1000, 1, RE_GUEST_MEM_EXEC,
        &pointer) == RE_GUEST_MEM_OK);

    requests[1].guest_base = 0x1000;
    requests[1].size = sizeof(code);
    assert(re_guest_memory_finalize_permissions(&memory, requests, 2,
        &failed) == RE_GUEST_PERMISSION_FINALIZE_E_DUPLICATE);
    assert(failed == 1);

    requests[0].final_permissions = 0;
    assert(re_guest_memory_finalize_permissions(&memory, requests, 1,
        &failed) == RE_GUEST_PERMISSION_FINALIZE_E_ARGUMENT);
    assert(failed == 0);

    requests[0].final_permissions = RE_GUEST_MEM_READ;
    requests[0].size = 0x100000000ULL;
    assert(re_guest_memory_finalize_permissions(&memory, requests, 1,
        &failed) == RE_GUEST_PERMISSION_FINALIZE_E_RANGE);
    assert(failed == 0);

    assert(re_guest_memory_finalize_permissions(&memory, NULL, 0, &failed)
        == RE_GUEST_PERMISSION_FINALIZE_OK);
    assert(failed == 0);
    puts("Guest permission finalization tests: PASS");
    return 0;
}
