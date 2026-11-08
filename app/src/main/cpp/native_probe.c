#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "re_android_asset_backend.h"
#include "re_android_jni_assets.h"

#define REPORT_CAP 2048
static void append(char *out, size_t cap, const char *line) {
    size_t used = strlen(out);
    if (used < cap - 1) (void)snprintf(out + used, cap - used, "%s\n", line);
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
