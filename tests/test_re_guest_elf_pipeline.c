#include "re_guest_map_transaction.h"
#include "re_guest_elf_materialize.h"
#include "re_guest_permission_finalize.h"
#include "re_guest_relocation_apply.h"
#include "re_guest_symbol_resolve.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint8_t backing[16] = {0};
    const uint8_t segment_bytes[4] = {0x11, 0x22, 0x33, 0x44};
    REGUESTMEMORY memory;
    REGUESTMEMMAPREQUEST map_request = {
        0x2000u, sizeof(backing), backing,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE, RE_GUEST_MEM_TYPE_NORMAL
    };
    REGUESTELFSEGMENT segment = {0x2000u, 0u, sizeof(segment_bytes), 8u};
    REGUESTSYMBOL provider = {
        "probe_symbol", 0x3000u, RE_GUEST_STB_GLOBAL,
        RE_GUEST_STV_DEFAULT, 1u, NULL, NULL
    };
    REGUESTSYMBOLREQUEST symbol_request = {"probe_symbol", NULL, NULL, 0u};
    REGUESTSYMBOLRESULT symbol_result = {0};
    REGUESTRELOCATION relocation = {
        0x2004u, RE_ARM_R_ABS32, 1u, 0x3000u, 0u, 5u, 1u, 1u
    };
    REGUESTMEMPERMISSIONREQUEST permission = {
        0x2000u, sizeof(backing), RE_GUEST_MEM_READ | RE_GUEST_MEM_EXEC
    };
    size_t failed_index = 0u;

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_transaction(&memory, &map_request, 1u,
        &failed_index, NULL) == RE_GUEST_MAP_TX_OK);
    assert(re_guest_elf_materialize(&memory, segment_bytes,
        sizeof(segment_bytes), &segment, 1u, &failed_index)
        == RE_GUEST_ELF_MATERIALIZE_OK);
    assert(memcmp(backing, segment_bytes, sizeof(segment_bytes)) == 0);
    assert(backing[4] == 0u && backing[7] == 0u);

    assert(re_guest_symbol_resolve(&symbol_request, &provider, 1u,
        &symbol_result) == RE_GUEST_SYMBOL_OK);
    assert(symbol_result.value == 0x3000u);
    relocation.symbol_value = symbol_result.value;
    assert(re_guest_relocations_apply(&memory, &relocation, 1u,
        &failed_index) == RE_GUEST_RELOC_APPLY_OK);
    assert(backing[4] == 0x05u && backing[5] == 0x30u &&
        backing[6] == 0u && backing[7] == 0u);

    assert(re_guest_memory_finalize_permissions(&memory, &permission,
        1u, &failed_index) == RE_GUEST_PERMISSION_FINALIZE_OK);
    {
        void *resolved = NULL;
        assert(re_guest_memory_resolve(&memory, 0x2000u, 1u,
            RE_GUEST_MEM_EXEC, &resolved) == RE_GUEST_MEM_OK);
        assert(re_guest_memory_resolve(&memory, 0x2000u, 1u,
            RE_GUEST_MEM_WRITE, &resolved) == RE_GUEST_MEM_E_PERMISSION);
    }
    assert(re_guest_memory_unmap(&memory, 0x2000u) == RE_GUEST_MEM_OK);
    assert(re_guest_memory_region_count(&memory) == 0u);
    puts("PASS: Stage 120 guest ELF map/materialize/symbol/relocation/permission pipeline");
    return 0;
}
