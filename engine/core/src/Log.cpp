#include "Log.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Elysium {

std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

void Log::Init() {
  // Pattern: [Timestamp] [LoggerName] [Level] Message
  spdlog::set_pattern("%^[%T] %n: %v%$");
  spdlog::set_level(spdlog::level::trace);

  // -----------------------------------------------------------------------
  // Sinks — shared between Core and Client loggers
  // -----------------------------------------------------------------------
  auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  // Rotating file sink: Elysium.log, 5 MB per file, 3 rotated files kept
  constexpr std::size_t maxSize = 5 * 1024 * 1024;
  constexpr std::size_t maxFiles = 3;
  auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      "Elysium.log", maxSize, maxFiles);

  std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};

  // -----------------------------------------------------------------------
  // Core logger
  // -----------------------------------------------------------------------
  s_CoreLogger =
      std::make_shared<spdlog::logger>("ELYSIUM", sinks.begin(), sinks.end());
  s_CoreLogger->set_level(spdlog::level::trace);
  s_CoreLogger->flush_on(spdlog::level::trace); // flush immediately on any log
  spdlog::register_logger(s_CoreLogger);

  // -----------------------------------------------------------------------
  // Client logger
  // -----------------------------------------------------------------------
  s_ClientLogger =
      std::make_shared<spdlog::logger>("APP", sinks.begin(), sinks.end());
  s_ClientLogger->set_level(spdlog::level::trace);
  s_ClientLogger->flush_on(spdlog::level::trace);
  spdlog::register_logger(s_ClientLogger);
}

} // namespace Elysium