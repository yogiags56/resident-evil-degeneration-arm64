#ifndef RE_ANDROID_JNI_ASSETS_H
#define RE_ANDROID_JNI_ASSETS_H
#include "re_android_asset_backend.h"
#include <jni.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    jobject asset_manager_global;
    AAssetManager *native_manager; /* borrowed from global Java ref */
    REAndroidAssetBackend backend_state;
    REFileAdapter adapter;
    uint8_t initialized;
} REAndroidJniAssets;

/* Call from JNI with a live Java AssetManager. One runtime thread owns this
 * object; do not race init/use/shutdown. */
REFileStatus re_android_jni_assets_init(JNIEnv *env, jobject java_asset_manager,
                                        REAndroidJniAssets *state);
/* Close all adapter streams before dropping the Java global reference. */
void re_android_jni_assets_shutdown(JNIEnv *env, REAndroidJniAssets *state);
#ifdef __cplusplus
}
#endif
#endif
