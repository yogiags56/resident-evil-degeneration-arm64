#include "re_guest_imports.h"
#include "re_s3e_file_api_audit.h"
#include "re_s3e_file_read_contract.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef enum {
    SYNTH_OPEN = 1,
    SYNTH_READ = 2,
    SYNTH_GET_CHAR = 3,
    SYNTH_CLOSE = 4
} SyntheticApi;

typedef struct {
    SyntheticApi api;
    uint32_t calls;
    uint32_t expected[4];
} SyntheticContext;

/* Test-only trampoline handler. The marker return is deliberately synthetic:
 * it verifies dispatcher register plumbing, not any Marmalade API semantics. */
static REGUESTIMPORTStatus synthetic_file_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory,
    void *user, REGUESTIMPORTResult *result) {
    SyntheticContext *ctx = (SyntheticContext *)user;
    uint32_t args[4];
    RES3EFileOpenArgs open_args;
    RES3EFileHandleArgs handle_args;
    RES3EFileReadArgs read_args;
    if (!frame || !memory || !ctx || !result) return RE_IMPORT_E_HANDLER;
    for (size_t i = 0; i < 4; ++i) args[i] = frame->r[i];
    switch (ctx->api) {
        case SYNTH_OPEN:
            if (re_s3e_file_open_capture(args, &open_args) != RE_S3E_FILE_API_OK ||
                open_args.path_guest_address != ctx->expected[0] ||
                open_args.mode_guest_address != ctx->expected[1])
                return RE_IMPORT_E_HANDLER;
            break;
        case SYNTH_READ:
            if (re_s3e_file_read_capture(args, &read_args) != RE_S3E_READ_CONTRACT_OK ||
                read_args.destination != ctx->expected[0] ||
                read_args.item_size != ctx->expected[1] ||
                read_args.item_count != ctx->expected[2] ||
                read_args.file_handle != ctx->expected[3] ||
                read_args.requested_bytes != 64u)
                return RE_IMPORT_E_HANDLER;
            break;
        case SYNTH_GET_CHAR:
            if (re_s3e_file_get_char_capture(args, &handle_args) != RE_S3E_FILE_API_OK ||
                handle_args.file_handle != ctx->expected[0])
                return RE_IMPORT_E_HANDLER;
            break;
        case SYNTH_CLOSE:
            if (re_s3e_file_close_capture(args, &handle_args) != RE_S3E_FILE_API_OK ||
                handle_args.file_handle != ctx->expected[0])
                return RE_IMPORT_E_HANDLER;
            break;
        default:
            return RE_IMPORT_E_HANDLER;
    }
    ctx->calls++;
    result->r0 = 0xA55A0000u | (uint32_t)ctx->api;
    result->r1 = 0x5AA50000u | (uint32_t)ctx->api;
    return RE_IMPORT_OK;
}

static void dispatch_one(REGUESTIMPORTTable *table, REGUESTCPU *cpu,
                         const REGUESTMEM *memory, uint32_t thumb_ptr,
                         uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3,
                         SyntheticApi api, uint32_t *assertions) {
    cpu->thumb = 1;
    cpu->r[15] = thumb_ptr & ~1u;
    cpu->r[14] = 0x50000021u;
    cpu->r[13] = 0x50000040u;
    cpu->r[0] = a0; cpu->r[1] = a1; cpu->r[2] = a2; cpu->r[3] = a3;
    assert(re_guest_import_dispatch(table, cpu, memory) == RE_IMPORT_OK); (*assertions)++;
    assert(cpu->r[0] == (0xA55A0000u | (uint32_t)api)); (*assertions)++;
    assert(cpu->r[1] == (0x5AA50000u | (uint32_t)api)); (*assertions)++;
    assert(cpu->r[15] == 0x50000020u && cpu->thumb == 1); (*assertions)++;
}

int main(void) {
    uint8_t bytes[128] = {0};
    REGUESTMEM memory = {bytes, sizeof(bytes), 0x50000000u};
    REGUESTIMPORTTable table;
    REGUESTCPU cpu;
    SyntheticContext contexts[4] = {
        {SYNTH_OPEN, 0, {0x60001000u, 0x60002000u, 0, 0}},
        {SYNTH_READ, 0, {0x60003000u, 4, 16, 0x1234u}},
        {SYNTH_GET_CHAR, 0, {0x1234u, 0, 0, 0}},
        {SYNTH_CLOSE, 0, {0x1234u, 0, 0, 0}}
    };
    uint32_t pointers[4] = {0};
    const uint32_t symbols[4] = {0xF001u, 0xF002u, 0xF003u, 0xF004u};
    uint32_t assertions = 0;

    assert(re_guest_import_table_init(&table, 0x51000000u) == RE_IMPORT_OK); assertions++;
    for (size_t i = 0; i < 4; ++i) {
        assert(re_guest_import_bind(&table, symbols[i], synthetic_file_handler,
                                    &contexts[i], &pointers[i]) == RE_IMPORT_OK);
        assertions++;
    }
    memset(&cpu, 0, sizeof(cpu));
    dispatch_one(&table, &cpu, &memory, pointers[0], 0x60001000u, 0x60002000u,
                 0, 0, SYNTH_OPEN, &assertions);
    dispatch_one(&table, &cpu, &memory, pointers[1], 0x60003000u, 4, 16,
                 0x1234u, SYNTH_READ, &assertions);
    dispatch_one(&table, &cpu, &memory, pointers[2], 0x1234u, 0, 0, 0,
                 SYNTH_GET_CHAR, &assertions);
    dispatch_one(&table, &cpu, &memory, pointers[3], 0x1234u, 0, 0, 0,
                 SYNTH_CLOSE, &assertions);

    /* A null destination is rejected by the read capture and becomes a
       handler failure; this does not manufacture a guest API error value. */
    cpu.thumb = 1; cpu.r[15] = pointers[1] & ~1u;
    cpu.r[14] = 0x50000021u; cpu.r[13] = 0x50000040u;
    cpu.r[0] = 0; cpu.r[1] = 4; cpu.r[2] = 16; cpu.r[3] = 0x1234u;
    assert(re_guest_import_dispatch(&table, &cpu, &memory) == RE_IMPORT_E_HANDLER);
    assertions++;

    for (size_t i = 0; i < 4; ++i) {
        assert(contexts[i].calls == 1u); assertions++;
    }
    assert(re_s3e_file_api_guest_imports_authorized() == 0); assertions++;
    assert(re_s3e_file_read_guest_return_is_authorized() == 0); assertions++;
    printf("Stage 96 synthetic file import dispatch: PASS (%u assertions)\n", assertions);
    return 0;
}
