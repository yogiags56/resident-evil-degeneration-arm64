#include "re_android_jni_assets.h"
#include <string.h>
#include <android/asset_manager_jni.h>

REFileStatus re_android_jni_assets_init(JNIEnv *env, jobject java_asset_manager,
                                        REAndroidJniAssets *state) {
    jobject global_ref;
    AAssetManager *native_manager;
    REFileBackend backend;
    REFileStatus status;
    if (!env || !java_asset_manager || !state) return RE_FILE_E_ARGUMENT;
    if (state->initialized || state->asset_manager_global) return RE_FILE_E_ARGUMENT;
    global_ref = (*env)->NewGlobalRef(env, java_asset_manager);
    if (!global_ref) return RE_FILE_E_BACKEND;
    native_manager = AAssetManager_fromJava(env, global_ref);
    if (!native_manager) {
        (*env)->DeleteGlobalRef(env, global_ref);
        return RE_FILE_E_BACKEND;
    }
    status = re_android_asset_backend_init(&state->backend_state,
                                           native_manager, &backend);
    if (status != RE_FILE_OK) {
        (*env)->DeleteGlobalRef(env, global_ref);
        return status;
    }
    status = re_file_adapter_init(&state->adapter, &backend,
                                  &state->backend_state);
    if (status != RE_FILE_OK) {
        (*env)->DeleteGlobalRef(env, global_ref);
        return status;
    }
    state->asset_manager_global = global_ref;
    state->native_manager = native_manager;
    state->initialized = 1u;
    return RE_FILE_OK;
}

void re_android_jni_assets_shutdown(JNIEnv *env, REAndroidJniAssets *state) {
    if (!env || !state) return;
    if (state->initialized) re_file_adapter_close_all(&state->adapter);
    state->native_manager = NULL;
    if (state->asset_manager_global) {
        (*env)->DeleteGlobalRef(env, state->asset_manager_global);
        state->asset_manager_global = NULL;
    }
    state->initialized = 0u;
    memset(&state->backend_state, 0, sizeof(state->backend_state));
    memset(&state->adapter, 0, sizeof(state->adapter));
}
