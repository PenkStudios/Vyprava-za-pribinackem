#ifndef AD_CPP
#define AD_CPP
#include <jni.h>

static JavaVM* jvm = NULL;
static jobject nativeLoaderInstance;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;

    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jvm = vm;

    jclass nativeLoaderClass = env->FindClass("com/zahon/pribinacek/NativeLoader");
    jmethodID getInstanceMethod = env->GetStaticMethodID(nativeLoaderClass, "getInstance",
                                                         "()Lcom/zahon/pribinacek/NativeLoader;");
    jobject instance = env->CallStaticObjectMethod(nativeLoaderClass, getInstanceMethod);
    nativeLoaderInstance = env->NewGlobalRef(instance);

    return JNI_VERSION_1_6;
}

void Release_Native_Loader(void) {
    JNIEnv* env;
    jvm->AttachCurrentThread(&env, NULL);

    if (nativeLoaderInstance != NULL) {
        env->DeleteGlobalRef(nativeLoaderInstance);
        nativeLoaderInstance = NULL;
    }

    jvm->DetachCurrentThread();
}

bool Show_Interstitial_Ad(void) {
    JNIEnv* env;
    jvm->AttachCurrentThread(&env, NULL);
    bool loaded_Return = false;

    if (nativeLoaderInstance != NULL) {
        jclass nativeLoaderClass = env->GetObjectClass(nativeLoaderInstance);
        jmethodID method = env->GetMethodID(nativeLoaderClass, "showAd", "()Z");
        jboolean loaded = env->CallBooleanMethod(nativeLoaderInstance, method);

        loaded_Return = (bool)loaded;
    }

    jvm->DetachCurrentThread();

    return loaded_Return;
}

/*
Vector2 Get_Display_Size(void) {
    JNIEnv* env;
    jvm->AttachCurrentThread(&env, NULL);
    float width, height;

    if (nativeLoaderInstance != NULL) {
        jclass nativeLoaderClass = env->GetObjectClass(nativeLoaderInstance);

        jmethodID width_Method = env->GetMethodID(nativeLoaderClass, "getDisplayWidth", "()I");
        jint display_Width = env->CallBooleanMethod(nativeLoaderInstance, width_Method);

        jmethodID height_Method = env->GetMethodID(nativeLoaderClass, "getDisplayHeight", "()I");
        jint display_Height = env->CallBooleanMethod(nativeLoaderInstance, height_Method);

        width = (float)display_Width;
        height = (float)display_Height;
    }

    jvm->DetachCurrentThread();
    return {width, height};
}
 */
#endif