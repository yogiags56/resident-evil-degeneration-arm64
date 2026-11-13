#include "re_guest_eabi_provider.h"
#include "re_guest_runtime.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t dbits(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
static double from_dregs(const REGUESTCPU *cpu) {
    uint64_t bits = ((uint64_t)cpu->r[1] << 32) | cpu->r[0];
    double value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
static uint32_t fbits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
static float from_freg(uint32_t bits) {
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
static void call_import(REGUESTRuntime *runtime, uint32_t pointer,
                        uint32_t return_address, uint32_t r0,
                        uint32_t r1, uint32_t r2, uint32_t r3) {
    runtime->cpu.r[15] = pointer & ~1u;
    runtime->cpu.thumb = 1u;
    runtime->cpu.r[14] = return_address | 1u;
    runtime->cpu.r[13] = 0x1000f000u;
    runtime->cpu.r[0] = r0;
    runtime->cpu.r[1] = r1;
    runtime->cpu.r[2] = r2;
    runtime->cpu.r[3] = r3;
    assert(re_guest_runtime_step(runtime) == RE_RUNTIME_OK);
    assert(runtime->cpu.r[15] == return_address);
}

int main(void) {
    static uint8_t guest[0x10000];
    static const char *const names[RE_GUEST_EABI_SYMBOL_COUNT] = {
        "__aeabi_d2f", "__aeabi_d2iz", "__aeabi_dadd",
        "__aeabi_dcmpeq", "__aeabi_dcmplt", "__aeabi_ddiv",
        "__aeabi_dmul", "__aeabi_dsub", "__aeabi_f2iz",
        "__aeabi_fadd", "__aeabi_fmul", "__aeabi_fsub",
        "__aeabi_i2d", "__aeabi_i2f", "__aeabi_idiv",
        "__aeabi_idivmod", "__aeabi_ldivmod", "__aeabi_lmul",
        "__aeabi_ui2d", "__aeabi_uidiv", "__aeabi_uidivmod"
    };
    REGUESTMEMORY memory;
    REGUESTMEMBUS bus;
    REGUESTIMPORTTable imports;
    REGUESTRuntime runtime;
    REGUESTEABIProvider provider;
    REGUESTELF32LINKSYMBOL symbols[RE_GUEST_EABI_SYMBOL_COUNT + 1u] = {0};
    REGUESTSYMBOL exports[RE_GUEST_EABI_SYMBOL_COUNT];
    size_t export_count = 0u, failed = 0u;
    uint64_t a, b;

    for (size_t i = 0; i < RE_GUEST_EABI_SYMBOL_COUNT; ++i) {
        (void)strcpy(symbols[i + 1u].name, names[i]);
        symbols[i + 1u].section_index = 0u;
        symbols[i + 1u].binding = RE_GUEST_STB_GLOBAL;
        symbols[i + 1u].type = 2u;
        symbols[i + 1u].visibility = RE_GUEST_STV_DEFAULT;
    }

    assert(re_guest_eabi_provider_init(&provider) == RE_GUEST_EABI_OK);
    assert(re_guest_import_table_init(&imports, 0x30000000u) == RE_IMPORT_OK);
    assert(re_guest_eabi_provider_bind_elf_symbols(&provider, &imports,
        symbols, RE_GUEST_EABI_SYMBOL_COUNT + 1u, exports,
        RE_GUEST_EABI_SYMBOL_COUNT, &export_count, &failed) == RE_GUEST_EABI_OK);
    assert(export_count == RE_GUEST_EABI_SYMBOL_COUNT);
    assert(failed == RE_GUEST_EABI_SYMBOL_COUNT + 1u);
    assert(imports.count == RE_GUEST_EABI_SYMBOL_COUNT);
    for (size_t i = 0; i < RE_GUEST_EABI_SYMBOL_COUNT; ++i) {
        assert(strcmp(exports[i].name, names[i]) == 0);
        assert(exports[i].version_library == NULL);
    }

    re_guest_memory_init(&memory);
    assert(re_guest_memory_map_ex(&memory, 0x10000000u, sizeof(guest), guest,
        RE_GUEST_MEM_READ | RE_GUEST_MEM_WRITE | RE_GUEST_MEM_EXEC,
        RE_GUEST_MEM_TYPE_NORMAL) == RE_GUEST_MEM_OK);
    re_guest_memory_bus_init(&bus, &memory);
    re_guest_runtime_init_mapped(&runtime, &bus, &imports, 0x40000000u,
                                 0x10000000u, 1);

    /* Double arithmetic and comparisons use core-register softfp arguments. */
    a = dbits(1.25); b = dbits(2.5);
    call_import(&runtime, exports[2].value, 0x10000020u,
        (uint32_t)a, (uint32_t)(a >> 32), (uint32_t)b, (uint32_t)(b >> 32));
    assert(from_dregs(&runtime.cpu) == 3.75);

    call_import(&runtime, exports[3].value, 0x10000040u,
        (uint32_t)a, (uint32_t)(a >> 32), (uint32_t)a, (uint32_t)(a >> 32));
    assert(runtime.cpu.r[0] == 1u);
    call_import(&runtime, exports[4].value, 0x10000060u,
        (uint32_t)a, (uint32_t)(a >> 32), (uint32_t)b, (uint32_t)(b >> 32));
    assert(runtime.cpu.r[0] == 1u);

    call_import(&runtime, exports[5].value, 0x10000080u,
        (uint32_t)b, (uint32_t)(b >> 32), (uint32_t)a, (uint32_t)(a >> 32));
    assert(from_dregs(&runtime.cpu) == 2.0);
    call_import(&runtime, exports[6].value, 0x100000a0u,
        (uint32_t)a, (uint32_t)(a >> 32), (uint32_t)b, (uint32_t)(b >> 32));
    assert(from_dregs(&runtime.cpu) == 3.125);
    call_import(&runtime, exports[7].value, 0x100000c0u,
        (uint32_t)a, (uint32_t)(a >> 32), (uint32_t)b, (uint32_t)(b >> 32));
    assert(from_dregs(&runtime.cpu) == -1.25);

    /* Floating conversions round toward zero for C-style integer casts. */
    a = dbits(-42.9);
    call_import(&runtime, exports[1].value, 0x100000e0u,
        (uint32_t)a, (uint32_t)(a >> 32), 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == -42);
    call_import(&runtime, exports[0].value, 0x10000100u,
        (uint32_t)dbits(3.5), (uint32_t)(dbits(3.5) >> 32), 0u, 0u);
    assert(from_freg(runtime.cpu.r[0]) == 3.5f);
    call_import(&runtime, exports[8].value, 0x10000120u,
        fbits(-3.75f), 0u, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == -3);

    call_import(&runtime, exports[9].value, 0x10000140u,
        fbits(1.5f), fbits(2.25f), 0u, 0u);
    assert(from_freg(runtime.cpu.r[0]) == 3.75f);
    call_import(&runtime, exports[10].value, 0x10000160u,
        fbits(1.5f), fbits(2.25f), 0u, 0u);
    assert(from_freg(runtime.cpu.r[0]) == 3.375f);
    call_import(&runtime, exports[11].value, 0x10000180u,
        fbits(1.5f), fbits(2.25f), 0u, 0u);
    assert(from_freg(runtime.cpu.r[0]) == -0.75f);

    call_import(&runtime, exports[12].value, 0x100001a0u,
        (uint32_t)-123, 0u, 0u, 0u);
    assert(from_dregs(&runtime.cpu) == -123.0);
    call_import(&runtime, exports[13].value, 0x100001c0u,
        (uint32_t)-123, 0u, 0u, 0u);
    assert(from_freg(runtime.cpu.r[0]) == -123.0f);
    call_import(&runtime, exports[18].value, 0x100001e0u,
        0xf0000000u, 0u, 0u, 0u);
    assert(from_dregs(&runtime.cpu) == 4026531840.0);

    /* 32-bit signed/unsigned division and value-in-registers divmod. */
    call_import(&runtime, exports[14].value, 0x10000200u,
        (uint32_t)-19, 5u, 0u, 0u);
    assert((int32_t)runtime.cpu.r[0] == -3);
    call_import(&runtime, exports[15].value, 0x10000220u,
        19u, 5u, 0u, 0u);
    assert(runtime.cpu.r[0] == 3u && runtime.cpu.r[1] == 4u);
    call_import(&runtime, exports[19].value, 0x10000240u,
        19u, 5u, 0u, 0u);
    assert(runtime.cpu.r[0] == 3u);
    call_import(&runtime, exports[20].value, 0x10000260u,
        19u, 5u, 0u, 0u);
    assert(runtime.cpu.r[0] == 3u && runtime.cpu.r[1] == 4u);
    call_import(&runtime, exports[15].value, 0x10000280u,
        19u, 0u, 0u, 0u);
    assert(runtime.cpu.r[0] == 0u && runtime.cpu.r[1] == 19u);

    /* 64-bit multiply and signed divmod return {q,r} in r0-r3. */
    a = UINT64_C(0x100000002);
    b = UINT64_C(3);
    call_import(&runtime, exports[17].value, 0x100002a0u,
        (uint32_t)a, (uint32_t)(a >> 32), (uint32_t)b, (uint32_t)(b >> 32));
    assert((((uint64_t)runtime.cpu.r[1] << 32) | runtime.cpu.r[0]) ==
           UINT64_C(0x300000006));

    a = (uint64_t)(int64_t)-19;
    b = (uint64_t)(int64_t)5;
    call_import(&runtime, exports[16].value, 0x100002c0u,
        (uint32_t)a, (uint32_t)(a >> 32), (uint32_t)b, (uint32_t)(b >> 32));
    assert((int64_t)(((uint64_t)runtime.cpu.r[1] << 32) | runtime.cpu.r[0]) == -3);
    assert((int64_t)(((uint64_t)runtime.cpu.r[3] << 32) | runtime.cpu.r[2]) == -4);

    /* Signed 64-bit divide-by-zero follows the documented zero quotient /
       original numerator remainder default policy. */
    b = 0u;
    call_import(&runtime, exports[16].value, 0x100002e0u,
        (uint32_t)a, (uint32_t)(a >> 32), 0u, 0u);
    assert(runtime.cpu.r[0] == 0u && runtime.cpu.r[1] == 0u);
    assert((int64_t)(((uint64_t)runtime.cpu.r[3] << 32) | runtime.cpu.r[2]) == -19);

    puts("Guest ARM EABI provider tests: PASS");
    return 0;
}
