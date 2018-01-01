#include <jni.h>
#include <string>
#include <string.h>
#include <android/log.h>

#include "../../../../../ping/SimplePingWrapper.h"

#define Log(...) __android_log_print(ANDROID_LOG_DEBUG, "Native", __VA_ARGS__)

using namespace std;

static jobject g_spobj = nullptr; //Java SimplePing Object
static jmethodID g_spmethod = nullptr;
static JNIEnv *g_env = nullptr;

static string jstring2string(jstring str)
{
    if (nullptr == g_env) {
        return "";
    }
    jsize length = g_env->GetStringUTFLength(str);
    if (!length) {
        return "";
    }

    const jclass stringClass = g_env->GetObjectClass(str);
    const jmethodID getBytes = g_env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray bytes = (jbyteArray) g_env->CallObjectMethod(str, getBytes, g_env->NewStringUTF("UTF-8"));

    length = g_env->GetArrayLength(bytes);
    jbyte* pBytes = g_env->GetByteArrayElements(bytes, nullptr);

    string cstr = string((char *)pBytes, length);
    g_env->ReleaseByteArrayElements(bytes, pBytes, JNI_ABORT);

    g_env->DeleteLocalRef(bytes);
    g_env->DeleteLocalRef(stringClass);
    return cstr;

}

static void ping(JNIEnv *env, jobject obj, jstring host, jint count) {
    g_spobj = obj;
    string strHost = jstring2string(host);
    if (strHost.empty()) return;
    SimplePingWrapper::sharedInstance()->ping(strHost, count);
}

JNINativeMethod methods[] = {
        {"ping", "(Ljava/lang/String;I)V", (void *) ping},
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    Log("JNI_Onload");
    if (jvm->GetEnv((void **) &g_env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }

    jclass clazz = g_env->FindClass("com/raythorn/simpleping/SimplePing");
    if (clazz == nullptr) {
        return JNI_ERR;
    }
    g_env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0]));
    Log("Register native method success!");

    g_spmethod = g_env->GetMethodID(clazz, "simplePing", "(ILjava/lang/String;)V");
    if (nullptr == g_spmethod) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *jvm, void *reserved) {
    g_env = nullptr;
    Log("JNI_OnUnload");
}

void _simplePing(int state, string message) {
    if (nullptr == g_spobj || nullptr == g_env || nullptr == g_spmethod) return;

    Log("%s", message.c_str());
//    char szMsg[256] = {0};
//    memcpy(szMsg, message.c_str(), message.length());
//    jstring msg = g_env->NewStringUTF(message.c_str());
//    g_env->CallVoidMethod(g_spobj, g_spmethod, state, msg);
}

