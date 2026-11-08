#ifndef STAGE88_MOCK_JNI_H
#define STAGE88_MOCK_JNI_H
#ifdef __cplusplus
extern "C" {
#endif
typedef void *jobject;
typedef void *jstring;
typedef int jint;
typedef const struct JNINativeInterface *JNIEnv;
#define JNIEXPORT
#define JNICALL
struct JNINativeInterface {
    jobject (*NewGlobalRef)(JNIEnv *, jobject);
    void (*DeleteGlobalRef)(JNIEnv *, jobject);
    jstring (*NewStringUTF)(JNIEnv *, const char *);
};
#ifdef __cplusplus
}
#endif
#endif
