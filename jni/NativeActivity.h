#include <jni.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <memory>

namespace app {

/**
 * Our saved state data.
 */
struct SavedState_ {
  float angle;
  int32_t x;
  int32_t y;
};

using SavedState = struct SavedState_;
using AndroidApp = struct android_app;

class NativeActivity {

public:
  NativeActivity(AndroidApp* app);
  ~NativeActivity();

  AndroidApp* app() { return android_app_; }
  SavedState saved_state() { return saved_state_; }

  static void OnHandleAppCmd(AndroidApp* app, int32_t cmd);
  static int32_t OnHandleAppInput(AndroidApp* app, AInputEvent* event);

  int32_t OnInitDisplay();
  void OnTermDisplay();
  void OnDraw();

  void OnGainedFocus();
  void OnLostFocus();

  void UpdateSavedState(int32_t x, int32_t y, bool animating);

  void MainLoop();

private:
  AndroidApp* android_app_;
  ASensorManager* sensor_manager_;
  const ASensor* accelerometer_sensor_;
  ASensorEventQueue* sensor_event_queue_;

  bool animating_;
  int32_t width_;
  int32_t height_;

  SavedState saved_state_;
  EGLDisplay egl_display_;
  EGLSurface egl_surface_;
  EGLContext egl_context_;
};

} // namespace app
