#include "NativeActivity.h"
#include "Log.h"

#include <functional>

namespace app {

NativeActivity::NativeActivity(AndroidApp* app)
  : android_app_(app),
    sensor_manager_(ASensorManager_getInstance()),
    accelerometer_sensor_(ASensorManager_getDefaultSensor(
      sensor_manager_, ASENSOR_TYPE_ACCELEROMETER)),
    sensor_event_queue_(ASensorManager_createEventQueue(
      sensor_manager_, app->looper, LOOPER_ID_USER, NULL, NULL)) {

  // Make sure glue isn't stripped.
  app_dummy();

  app->userData = this;
  app->onAppCmd = &NativeActivity::OnHandleAppCmd;
  app->onInputEvent = &NativeActivity::OnHandleAppInput;

  // We are starting with a previous saved state; restore from it.
  if (app->savedState != NULL) {
    saved_state_ = *(static_cast<SavedState*>(app->savedState));
  }
}

NativeActivity::~NativeActivity() {
}

int NativeActivity::OnInitDisplay() {
  if (android_app_->window == NULL)
    return -2;

  // initialize OpenGL ES and EGL

  /*
   * Here specify the attributes of the desired configuration.
   * Below, we select an EGLConfig with at least 8 bits per color
   * component compatible with on-screen windows
   */
  const EGLint attribs[] = {
          EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
          EGL_BLUE_SIZE, 8,
          EGL_GREEN_SIZE, 8,
          EGL_RED_SIZE, 8,
          EGL_NONE
  };
  EGLint w, h, dummy, format;
  EGLint numConfigs;
  EGLConfig config;
  EGLSurface surface;
  EGLContext context;

  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  eglInitialize(display, 0, 0);

  /* Here, the application chooses the configuration it desires. In this
   * sample, we have a very simplified selection process, where we pick
   * the first EGLConfig that matches our criteria */
  eglChooseConfig(display, attribs, &config, 1, &numConfigs);

  /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
   * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
   * As soon as we picked a EGLConfig, we can safely reconfigure the
   * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
  eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);

  ANativeWindow_setBuffersGeometry(android_app_->window, 0, 0, format);

  surface = eglCreateWindowSurface(display, config, android_app_->window, NULL);
  context = eglCreateContext(display, config, NULL, NULL);

  if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
      //LOGW("Unable to eglMakeCurrent");
      return -1;
  }

  eglQuerySurface(display, surface, EGL_WIDTH, &w);
  eglQuerySurface(display, surface, EGL_HEIGHT, &h);

  egl_display_ = display;
  egl_context_ = context;
  egl_surface_ = surface;
  width_ = w;
  height_ = h;
  saved_state_.angle = 0;

  // Initialize GL state.
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glEnable(GL_CULL_FACE);
  glShadeModel(GL_SMOOTH);
  glDisable(GL_DEPTH_TEST);

  // Draw a frame.
  OnDraw();
  return 0;
}

void NativeActivity::OnDraw() {
  if (egl_display_ == EGL_NO_DISPLAY)
    return;
  
  // Just fill the screen with a color.
  glClearColor(((float)saved_state_.x)/width_,
               saved_state_.angle,
               ((float)saved_state_.y)/height_, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  eglSwapBuffers(egl_display_, egl_surface_);
}

void NativeActivity::OnTermDisplay() {
  if (egl_display_ != EGL_NO_DISPLAY) {
    eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (egl_context_ != EGL_NO_CONTEXT) {
        eglDestroyContext(egl_display_, egl_context_);
    }
    if (egl_surface_ != EGL_NO_SURFACE) {
        eglDestroySurface(egl_display_, egl_surface_);
    }
    eglTerminate(egl_display_);
  }
  animating_ = false;
  egl_display_ = EGL_NO_DISPLAY;
  egl_context_ = EGL_NO_CONTEXT;
  egl_surface_ = EGL_NO_SURFACE;
}

void NativeActivity::OnGainedFocus() {
  // When our app gains focus, we start monitoring the accelerometer.
  // accelerometer_sensor_ is bind to app->looper with LOOPER_ID_USER use sensor_event_queue.
  // start accelerate, main loop will receiver looper_id_user event.
  if (accelerometer_sensor_ != NULL) {
    ASensorEventQueue_enableSensor(sensor_event_queue_, accelerometer_sensor_);
    // We'd like to get 60 events per second (in us).
    ASensorEventQueue_setEventRate(sensor_event_queue_, accelerometer_sensor_, (1000L/60)*1000);
  }
}

void NativeActivity::OnLostFocus() {
  // When our app loses focus, we stop monitoring the accelerometer.
  // This is to avoid consuming battery while not being used.
  if (accelerometer_sensor_ != NULL) {
    ASensorEventQueue_disableSensor(sensor_event_queue_, accelerometer_sensor_);
  }
  // Also stop animating.
  animating_ = false;
  OnDraw();
}

/**
 * static method, process the next input event.
 */
int32_t NativeActivity::OnHandleAppInput(AndroidApp* app, AInputEvent* event) {
  NativeActivity* activity = static_cast<NativeActivity*>(app->userData);
  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    activity->UpdateSavedState(
        AMotionEvent_getX(event, 0),
        AMotionEvent_getY(event, 0),
        true);
    return 1;
  }
  return 0;
}

/**
 * static method, process the next main command, should not be blocked.
 */
void NativeActivity::OnHandleAppCmd(AndroidApp* app, int32_t cmd) {
  NativeActivity* activity = static_cast<NativeActivity*>(app->userData);
  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      LOG(INFO) << "NativeActivity: APP_CMD_SAVE_STATE";
      // The system has asked us to save our current state.  Do so.
      activity->app()->savedState = static_cast<void*>(new SavedState(activity->saved_state()));
      activity->app()->savedStateSize = sizeof(SavedState);
      break;
    case APP_CMD_INIT_WINDOW:
      LOG(INFO) << "NativeActivity: APP_CMD_INIT_WINDOW";
      // The window is being shown, get it ready.
      activity->OnInitDisplay();
      break;
    case APP_CMD_TERM_WINDOW:
      LOG(INFO) << "NativeActivity: APP_CMD_TERM_WINDOW";
      // The window is being hidden or closed, clean it up.
      activity->OnTermDisplay();
      break;
    case APP_CMD_GAINED_FOCUS:
      LOG(INFO) << "NativeActivity: APP_CMD_GAINED_FOCUS";
      activity->OnGainedFocus();
      break;
    case APP_CMD_LOST_FOCUS:
      LOG(INFO) << "NativeActivity: APP_CMD_LOST_FOCUS";
      activity->OnLostFocus();
      break;
    case APP_CMD_WINDOW_REDRAW_NEEDED:
      activity->OnDraw();
      break;
    case APP_CMD_LOW_MEMORY:
      break;
    case APP_CMD_START:
      break;
    case APP_CMD_RESUME:
      break;
    case APP_CMD_PAUSE:
      break;
    case APP_CMD_STOP:
      break;
    case APP_CMD_DESTROY:
      break;
  }
}

void NativeActivity::UpdateSavedState(int32_t x, int32_t y, bool animating) {
  saved_state_.x = x;
  saved_state_.y = y;
  animating_ = animating;
}

void NativeActivity::MainLoop() {
  // loop waiting for stuff to do.
  while (true) {
    // Read all pending events.
    int ident;
    int events;
    struct android_poll_source* source;

    // If not animating, we will block forever waiting for events.
    // If animating, we loop until all events are read, then continue
    // to draw the next frame of animation.
    while ((ident=ALooper_pollAll(animating_ ? 0 : -1, NULL, &events,
        (void**)&source)) >= 0) {

      // LOOPER_ID_MAIN = 1, LOOPER_ID_INPUT = 2, LOOPER_ID_USER = 3, see android_native_app_glue.h
      //LOG(INFO) << "NativeActivity: MainLoop event " << ident;
      // Process this event. ID_MAIN in AppCmd, ID_INPUT in AppInput.
      if (source != NULL) {
        source->process(android_app_, source);
      }

      // accelerometer event.
      if (ident == LOOPER_ID_USER && accelerometer_sensor_ != NULL) {
        ASensorEvent event;
        while (ASensorEventQueue_getEvents(sensor_event_queue_,
              &event, 1) > 0) {
        }
      }

      // Check if we are exiting.
      if (android_app_->destroyRequested != 0) {
        OnTermDisplay();
        return;
      }
    }

    if (animating_) {
      // Done with events; draw next animation frame.
      saved_state_.angle += .01f;
      if (saved_state_.angle > 1) {
        saved_state_.angle = 0;
      }
      // Drawing is throttled to the screen update rate, so there
      // is no need to do timing here.
      OnDraw();
    }
  }
}

} // namespace app.
