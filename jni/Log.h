#include <sstream>

using Level = int;
const Level INFO = 0;
const Level WARNING = 1;
const Level ERROR = 2;
const Level FATAL = 3;

// usage LOG(INFO) << "main: hello world";
#define LOG(LEVEL) log::LogMessage(__FILE__, __LINE__, LEVEL).stream()

namespace log {
// LogMessage.
class LogMessage {
public:
  LogMessage(const char* file, int line, int level);
  ~LogMessage();

  std::ostream& stream() { return stream_; }
private:
  void Init(const char* file, int line);
  const char* file_;
  const int line_;
  const int level_;
  std::ostringstream stream_;
};
}
