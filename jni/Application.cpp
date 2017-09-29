#include "NativeActivity.h"
/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */

// 使用AndroidManifest 来定义另一个Activity?? 那如何启动另一个activity呢？
// 每个Activity一个android_main ?? 应该不是。。。
// 参考 https://github.com/chicken-mobile/android-ndk/blob/master/android-native-app/android_native_app_glue.h
void android_main(struct android_app* app) {

    app::NativeActivity activity(app);
    activity.MainLoop();
}
