#ifndef STAGE88_MOCK_ASSET_MANAGER_JNI_H
#define STAGE88_MOCK_ASSET_MANAGER_JNI_H
#include <jni.h>
#include <android/asset_manager.h>
AAssetManager *AAssetManager_fromJava(JNIEnv *, jobject);
#endif
