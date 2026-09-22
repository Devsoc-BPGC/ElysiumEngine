#include "EditorLogSink.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Elysium {

static std::shared_ptr<EditorLogSink> s_EditorLogSink = nullptr;

EditorLogSink::EditorLogSink(size_t maxMessages) : m_maxMessages(maxMessages) {}

std::vector<LogMessage> EditorLogSink::GetMessages() {
  std::lock_guard<std::mutex> lock(mutex_);
  return {m_messages.begin(), m_messages.end()};
}

void EditorLogSink::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  m_messages.clear();
}

void EditorLogSink::sink_it_(const spdlog::details::log_msg &msg) {
  // Format message payload
  spdlog::memory_buf_t formatted;
  base_sink<std::mutex>::formatter_->format(msg, formatted);
  std::string formattedStr(formatted.data(), formatted.size());

  // Remove trailing newline if present
  while (!formattedStr.empty() &&
         (formattedStr.back() == '\n' || formattedStr.back() == '\r')) {
    formattedStr.pop_back();
  }

  // Format time string HH:MM:SS
  auto timePoint = msg.time;
  auto timeT = std::chrono::system_clock::to_time_t(timePoint);
  std::tm tmBuffer{};
#if defined(_WIN32)
  localtime_s(&tmBuffer, &timeT);
#else
  localtime_r(&timeT, &tmBuffer);
#endif

  std::ostringstream ss;
  ss << std::put_time(&tmBuffer, "%H:%M:%S");

  if (m_messages.size() >= m_maxMessages) {
    m_messages.pop_front();
  }

  m_messages.push_back({msg.level, formattedStr, ss.str()});
}

void EditorLogSink::flush_() {}

std::shared_ptr<EditorLogSink> GetEditorLogSink() {
  if (!s_EditorLogSink) {
    s_EditorLogSink = std::make_shared<EditorLogSink>();
  }
  return s_EditorLogSink;
}

} // namespace Elysium
