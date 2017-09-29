#include "Log.h"

#include <sys/time.h>
#include <unistd.h>
#include <iomanip>
#include <utility>

#include <android/log.h>

#define LOGI(TAG, ...) ((void)__android_log_write(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define LOGW(TAG, ...) ((void)__android_log_write(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define LOGE(TAG, ...) ((void)__android_log_write(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))
#define LOGF(TAG, ...) ((void)__android_log_write(ANDROID_LOG_FATAL, TAG, __VA_ARGS__))

namespace log {

LogMessage::LogMessage(const char* file, int line, int level)
    : file_(file), line_(line), level_(level) {
  Init(file, line);
}

LogMessage::~LogMessage() {
  stream_ << std::endl;

  if (level_ == INFO) {
    LOGI(file_, stream_.str().c_str());
  }
}

void LogMessage::Init(const char* file, int line) {

  stream_ << "[pid=" << getpid() << ", tid="
          << gettid() << ", time=";

  timeval tv;
  gettimeofday(&tv, nullptr);
  time_t t = tv.tv_sec;
  struct tm local_time;
  localtime_r(&t, &local_time);
  struct tm* tm_time = &local_time;
  stream_ << std::setfill('0')
          << std::setw(2) << 1 + tm_time->tm_mon
          << std::setw(2) << tm_time->tm_mday
          << ':'
          << std::setw(2) << tm_time->tm_hour
          << std::setw(2) << tm_time->tm_min
          << std::setw(2) << tm_time->tm_sec
          << '.'
          << std::setw(6) << tv.tv_usec;

  stream_ << "] ";
}

}
