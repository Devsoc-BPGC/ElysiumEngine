#ifndef EDITORLOGSINK_HPP
#define EDITORLOGSINK_HPP

#include <deque>
#include <memory>
#include <mutex>
#include <spdlog/common.h>
#include <spdlog/sinks/base_sink.h>
#include <string>
#include <vector>

namespace Elysium {

struct LogMessage {
  spdlog::level::level_enum level;
  std::string message;
  std::string timeStr;
};

class EditorLogSink : public spdlog::sinks::base_sink<std::mutex> {
public:
  explicit EditorLogSink(size_t maxMessages = 1000);

  std::vector<LogMessage> GetMessages();
  void Clear();

protected:
  void sink_it_(const spdlog::details::log_msg &msg) override;
  void flush_() override;

private:
  size_t m_maxMessages;
  std::deque<LogMessage> m_messages;
};

std::shared_ptr<EditorLogSink> GetEditorLogSink();

} // namespace Elysium

#endif // EDITORLOGSINK_HPP
