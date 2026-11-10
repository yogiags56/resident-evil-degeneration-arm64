#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "re_android_asset_backend.h"
#include "re_android_jni_assets.h"
#include "re_guest_runtime.h"
#include "re_guest_map_transaction.h"
#include "re_guest_elf_materialize.h"
#include "re_guest_permission_finalize.h"
#include "re_guest_relocation_apply.h"
#include "re_guest_symbol_resolve.h"
#include "re_guest_elf32_image.h"
#include "re_guest_elf32_dynamic.h"

#define REPORT_CAP 2048

static REGUESTIMPORTStatus probe_add_handler(
    const S3EAAPCS32Frame *frame, const REGUESTMEM *memory, void *user,
    REGUESTIMPORTResult *result) {
    (void)memory;
    (void)user;
    if (!frame || !result) return RE_IMPORT_E_HANDLER;
    result->r0 = frame->r[0] + frame->r[1];
    result->r1 = 0u;
    return RE_IMPORT_OK;
}

static void append(char *out, size_t cap, const char *line) {
    size_t used = strlen(out);
    if (used < cap - 1) (void)snprintf(out + used, cap - used, "%s\n", line);
}


static int probe_elf32_program_header_parser(char *report, size_t cap) {
    uint8_t elf[84] = {0};
    REGUESTELF32IMAGE image;
    size_t failed = 0u;
    elf[0]=0x7fu; elf[1]='E'; elf[2]='L'; elf[3]='F';
    elf[4]=1u; elf[5]=1u; elf[6]=1u;
    elf[16]=(uint8_t)RE_GUEST_ELF32_ET_EXEC;
    elf[18]=(uint8_t)RE_GUEST_ELF32_EM_ARM;
    elf[20]=1u; elf[24]=0x00u; elf[25]=0x10u;
    elf[28]=52u; elf[40]=52u; elf[42]=32u; elf[44]=1u;
    elf[52]=(uint8_t)RE_GUEST_ELF32_PT_LOAD;
    elf[60]=0x00u; elf[61]=0x10u; elf[64]=0x00u; elf[65]=0x10u;
    elf[72]=4u; elf[76]=(uint8_t)(RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_X);
    elf[80]=0x00u; elf[81]=0x10u;
    if (re_guest_elf32_parse_image(elf,sizeof(elf),&image,&failed)==RE_GUEST_ELF32_OK &&
        image.type==RE_GUEST_ELF32_ET_EXEC && image.machine==RE_GUEST_ELF32_EM_ARM &&
        image.load_count==1u && image.entry_in_executable_load && failed==1u) {
        append(report,cap,"PASS: ELF32 ARM program-header parser");
        return 0;
    }
    append(report,cap,"FAIL: ELF32 ARM program-header parser");
    return 1;
}

static void probe_put_u32le(uint8_t *p, uint32_t value) {
    p[0]=(uint8_t)value; p[1]=(uint8_t)(value>>8);
    p[2]=(uint8_t)(value>>16); p[3]=(uint8_t)(value>>24);
}

static int probe_elf32_dynamic_parser(char *report, size_t cap) {
    uint8_t elf[256] = {0};
    REGUESTELF32IMAGE image;
    REGUESTELF32DYNAMIC dynamic;
    size_t failed = 0u;
    elf[0]=0x7fu; elf[1]='E'; elf[2]='L'; elf[3]='F';
    elf[4]=1u; elf[5]=1u; elf[6]=1u;
    elf[16]=(uint8_t)RE_GUEST_ELF32_ET_EXEC;
    elf[18]=(uint8_t)RE_GUEST_ELF32_EM_ARM;
    elf[20]=1u; elf[24]=0x00u; elf[25]=0x10u;
    elf[28]=52u; elf[40]=52u; elf[42]=32u; elf[44]=2u;
    probe_put_u32le(elf+52,RE_GUEST_ELF32_PT_LOAD);
    probe_put_u32le(elf+60,0x1000u); probe_put_u32le(elf+64,0x1000u);
    probe_put_u32le(elf+68,sizeof(elf)); probe_put_u32le(elf+72,sizeof(elf));
    probe_put_u32le(elf+76,RE_GUEST_ELF32_PF_R|RE_GUEST_ELF32_PF_X);
    probe_put_u32le(elf+80,0x1000u);
    probe_put_u32le(elf+84,RE_GUEST_ELF32_PT_DYNAMIC);
    probe_put_u32le(elf+88,160u); probe_put_u32le(elf+92,0x10a0u);
    probe_put_u32le(elf+100,48u); probe_put_u32le(elf+104,48u);
    probe_put_u32le(elf+108,RE_GUEST_ELF32_PF_R); probe_put_u32le(elf+112,4u);
    probe_put_u32le(elf+160,RE_GUEST_ELF32_DT_STRTAB); probe_put_u32le(elf+164,0x10d0u);
    probe_put_u32le(elf+168,RE_GUEST_ELF32_DT_STRSZ); probe_put_u32le(elf+172,32u);
    probe_put_u32le(elf+176,RE_GUEST_ELF32_DT_NEEDED); probe_put_u32le(elf+180,1u);
    probe_put_u32le(elf+184,RE_GUEST_ELF32_DT_NEEDED); probe_put_u32le(elf+188,9u);
    probe_put_u32le(elf+192,RE_GUEST_ELF32_DT_SONAME); probe_put_u32le(elf+196,17u);
    probe_put_u32le(elf+200,RE_GUEST_ELF32_DT_NULL);
    (void)memcpy(elf+208,"\0libc.so\0libm.so\0libroot.so\0",28u);
    if (re_guest_elf32_parse_image(elf,sizeof(elf),&image,&failed)!=RE_GUEST_ELF32_OK ||
        re_guest_elf32_parse_dynamic(elf,sizeof(elf),&image,&dynamic,&failed)!=
            RE_GUEST_ELF32_DYNAMIC_OK ||
        !dynamic.present || !dynamic.has_soname || dynamic.needed_count!=2u ||
        strcmp(dynamic.needed[0],"libc.so")!=0 ||
        strcmp(dynamic.needed[1],"libm.so")!=0 ||
        strcmp(dynamic.soname,"libroot.so")!=0) {
        append(report,cap,"FAIL: ELF32 PT_DYNAMIC dependency parser");
        return 1;
    }
    append(report,cap,"PASS: ELF32 PT_DYNAMIC/DT_NEEDED/DT_SONAME parser");
    return 0;
}

JNIEXPORT jstring JNICALL
Java_org_example_redstage_MainActivity_nativeProbe(JNIEnv *env, jobject self,
                                                       jobject java_manager) {
    char report[REPORT_CAP] = {0};
    char line[256];
    REAndroidJniAssets lifecycle;
    REAndroidAssetBackend backend_state;
    REFileBackend backend;
    AAssetManager *manager;
    AAsset *asset;
    unsigned char header[16] = {0};
    int got;
    int failures = 0;
    (void)self;
    memset(&lifecycle, 0, sizeof(lifecycle));
    memset(&backend_state, 0, sizeof(backend_state));

    if (re_android_jni_assets_init(env, java_manager, &lifecycle) != RE_FILE_OK) {
        append(report, sizeof(report), "FAIL: JNI AssetManager lifecycle init");
        return (*env)->NewStringUTF(env, report);
    }
    append(report, sizeof(report), "PASS: JNI global-reference lifecycle initialized");
    manager = lifecycle.native_manager;

    /* Execute one real guest Thumb instruction through the runtime coordinator. */
    {
        uint8_t guest_code[4] = {0x2A, 0x20, 0x00, 0x00}; /* MOVS r0,#42 */
        REGUESTMEM guest_memory = {guest_code, sizeof(guest_code), 0x1000u};
        REGUESTIMPORTTable guest_imports;
        REGUESTRuntime guest_runtime;
        uint32_t probe_trampoline = 0u;
        if (re_guest_import_table_init(&guest_imports, 0x60000000u) != RE_IMPORT_OK ||
            re_guest_import_bind(&guest_imports, 0x7FFFu, probe_add_handler,
                                 NULL, &probe_trampoline) != RE_IMPORT_OK) {
            append(report, sizeof(report), "FAIL: guest import table/handler initialization");
            failures++;
        } else {
            re_guest_runtime_init(&guest_runtime, &guest_memory, &guest_imports,
                                  0x6000F000u, 0x1000u, 1);
            if (re_guest_runtime_step(&guest_runtime) == RE_RUNTIME_OK &&
                guest_runtime.cpu.r[0] == 42u &&
                guest_runtime.cpu_instructions == 1u &&
                guest_runtime.dispatched_imports == 0u) {
                append(report, sizeof(report),
                       "PASS: AArch32 Thumb instruction executed on Android ARM64");
            } else {
                append(report, sizeof(report), "FAIL: guest CPU/runtime smoke execution");
                failures++;
            }
            guest_runtime.cpu.r[0] = 19u;
            guest_runtime.cpu.r[1] = 23u;
            guest_runtime.cpu.r[14] = 0x1003u;
            guest_runtime.cpu.r[15] = probe_trampoline & ~1u;
            guest_runtime.cpu.thumb = 1u;
            if (re_guest_runtime_step(&guest_runtime) == RE_RUNTIME_OK &&
                guest_runtime.cpu.r[0] == 42u &&
                guest_runtime.cpu.r[15] == 0x1002u &&
                guest_runtime.dispatched_imports == 1u &&
                guest_runtime.cpu_instructions == 1u) {
                append(report, sizeof(report),
                       "PASS: AAPCS32 import trampoline dispatched and returned correctly");
            } else {
                append(report, sizeof(report), "FAIL: guest import dispatcher smoke execution");
                failures++;
            }
        }
    }

    failures += probe_elf32_program_header_parser(report, sizeof(report));

    failures += probe_elf32_dynamic_parser(report, sizeof(report));

    /*
     * Exercise the Stage 120 guest ELF primitives as one bounded transaction.
     * This is a synthetic smoke test only; it does not load the proprietary game.
     */
    {
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
        REGUESTSYMBOLREQUEST symbol_request = {
            "probe_symbol", NULL, NULL, 0u
        };
        REGUESTSYMBOLRESULT symbol_result;
        REGUESTRELOCATION relocation = {
            0x2004u, RE_ARM_R_ABS32, 1u, 0x3000u, 0u, 5u, 1u, 1u
        };
        REGUESTMEMPERMISSIONREQUEST permission = {
            0x2000u, sizeof(backing), RE_GUEST_MEM_READ | RE_GUEST_MEM_EXEC
        };
        size_t failed_index = 0u;
        int ok = 1;

        re_guest_memory_init(&memory);
        if (re_guest_memory_map_transaction(&memory, &map_request, 1u,
                &failed_index, NULL) != RE_GUEST_MAP_TX_OK) {
            append(report, sizeof(report), "FAIL: Stage 120 guest map transaction");
            failures++;
            ok = 0;
        }
        if (ok && re_guest_elf_materialize(&memory, segment_bytes,
                sizeof(segment_bytes), &segment, 1u, &failed_index)
                != RE_GUEST_ELF_MATERIALIZE_OK) {
            append(report, sizeof(report), "FAIL: Stage 120 ELF segment materialization");
            failures++;
            ok = 0;
        }
        if (ok && (memcmp(backing, segment_bytes, sizeof(segment_bytes)) != 0 ||
                backing[4] != 0u || backing[7] != 0u)) {
            append(report, sizeof(report), "FAIL: Stage 120 ELF copy/BSS zero-fill");
            failures++;
            ok = 0;
        }
        if (ok && re_guest_symbol_resolve(&symbol_request, &provider, 1u,
                &symbol_result) != RE_GUEST_SYMBOL_OK) {
            append(report, sizeof(report), "FAIL: Stage 120 guest symbol resolution");
            failures++;
            ok = 0;
        }
        if (ok) {
            relocation.symbol_value = symbol_result.value;
            if (re_guest_relocations_apply(&memory, &relocation, 1u,
                    &failed_index) != RE_GUEST_RELOC_APPLY_OK ||
                    backing[4] != 0x05u || backing[5] != 0x30u ||
                    backing[6] != 0u || backing[7] != 0u) {
                append(report, sizeof(report), "FAIL: Stage 120 ARM relocation application");
                failures++;
                ok = 0;
            }
        }
        if (ok && re_guest_memory_finalize_permissions(&memory, &permission,
                1u, &failed_index) != RE_GUEST_PERMISSION_FINALIZE_OK) {
            append(report, sizeof(report), "FAIL: Stage 120 final guest memory permissions");
            failures++;
            ok = 0;
        }
        if (ok) {
            append(report, sizeof(report),
                "PASS: Stage 120 map/materialize/symbol/relocation/permission pipeline");
        }
        (void)re_guest_memory_unmap(&memory, 0x2000u);
    }

    {
        const char *required[] = {"main.dz", "biohazard.s3e", "s3e.icf"};
        for (size_t i = 0; i < sizeof(required) / sizeof(required[0]); ++i) {
            AAsset *required_asset = AAssetManager_open(
                manager, required[i], AASSET_MODE_STREAMING);
            if (!required_asset) {
                (void)snprintf(line, sizeof(line),
                    "FAIL: required APK asset missing: %s", required[i]);
                append(report, sizeof(report), line);
                failures++;
            } else {
                (void)snprintf(line, sizeof(line),
                    "PASS: required APK asset packaged: %s", required[i]);
                append(report, sizeof(report), line);
                AAsset_close(required_asset);
            }
        }
    }

    if (re_android_asset_backend_init(&backend_state, manager, &backend) != RE_FILE_OK) {
        append(report, sizeof(report), "FAIL: Android asset backend init");
        failures++;
    } else {
        void *stream = backend.open(&backend_state, "main.dz", "rb");
        if (!stream) {
            append(report, sizeof(report), "FAIL: open main.dz"); failures++;
        } else {
            int64_t n = backend.read(&backend_state, stream, header, sizeof(header));
            if (n != (int64_t)sizeof(header)) {
                append(report, sizeof(report), "FAIL: read 16-byte main.dz header"); failures++;
            } else if (memcmp(header, "DTRZ", 4) != 0) {
                append(report, sizeof(report), "FAIL: main.dz DTRZ magic mismatch"); failures++;
            } else {
                (void)snprintf(line, sizeof(line),
                    "PASS: main.dz opened; 16 bytes read; DTRZ magic valid; bytes=%02X %02X %02X %02X",
                    header[0], header[1], header[2], header[3]);
                append(report, sizeof(report), line);
            }
            backend.close(&backend_state, stream);
        }
        stream = backend.open(&backend_state, "__stage87_missing__.bin", "rb");
        if (stream) { backend.close(&backend_state, stream); append(report, sizeof(report), "FAIL: missing asset unexpectedly opened"); failures++; }
        else append(report, sizeof(report), "PASS: missing asset rejected");

        stream = backend.open(&backend_state, "main.dz", "rb");
        if (!stream) { append(report, sizeof(report), "FAIL: reopen main.dz"); failures++; }
        else {
            unsigned char scratch[32];
            int64_t first = backend.read(&backend_state, stream, scratch, sizeof(scratch));
            int64_t rest = backend.read(&backend_state, stream, scratch, 0);
            if (first != 32 || rest != 0) { append(report, sizeof(report), "FAIL: zero-length read contract"); failures++; }
            else append(report, sizeof(report), "PASS: reopen and zero-length read");
            backend.close(&backend_state, stream);
        }
    }

    /* Independent direct AAsset check confirms this APK's assets/ root mapping. */
    asset = AAssetManager_open(manager, "main.dz", AASSET_MODE_STREAMING);
    if (!asset) { append(report, sizeof(report), "FAIL: direct AAssetManager open main.dz"); failures++; }
    else {
        got = AAsset_read(asset, header, 4);
        if (got == 4 && memcmp(header, "DTRZ", 4) == 0)
            append(report, sizeof(report), "PASS: direct NDK AAssetManager read");
        else { append(report, sizeof(report), "FAIL: direct NDK asset read/magic"); failures++; }
        AAsset_close(asset);
    }
    re_android_jni_assets_shutdown(env, &lifecycle);
    if (!lifecycle.initialized && !lifecycle.asset_manager_global)
        append(report, sizeof(report), "PASS: shutdown released handles and JNI global reference");
    else { append(report, sizeof(report), "FAIL: shutdown lifecycle state"); failures++; }
    (void)snprintf(line, sizeof(line), "\nRESULT: %s (%d failures)", failures ? "FAIL" : "PASS", failures);
    append(report, sizeof(report), line);
    return (*env)->NewStringUTF(env, report);
}
