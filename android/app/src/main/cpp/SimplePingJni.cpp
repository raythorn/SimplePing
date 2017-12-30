#include <jni.h>
#include <string>

using namespace std;

static void ping(JNIEnv *evn, jobject obj, jstring host, jint count, jint timeout) {

}

JNINativeMethod methods[] = {
        {"ping", "(Ljava/lang/String;II)V", (void*)ping},
};

JNIEXPORT jint JNICALL JNI_Onload(JavaVM *jvm, void *reserved) {

    JNIEnv *env;
    if (jvm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }

    jclass claz = env->FindClass("com/raythorn/simpleping/SimplePing");
    env->RegisterNatives(claz, methods, sizeof(methods)/sizeof(methods[0]));

    return JNI_VERSION_1_4;
}

void _simplePingMessage(int state, std::string message) {

}

void _simplePingResult() {

}

void _simplePingFail(int code, std::string error) {

}
