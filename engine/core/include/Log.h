#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace Elysium {

class Log {
public:
  static void Init();

  inline static std::shared_ptr<spdlog::logger> &GetCoreLogger() {
    return s_CoreLogger;
  }
  inline static std::shared_ptr<spdlog::logger> &GetClientLogger() {
    return s_ClientLogger;
  }

private:
  static std::shared_ptr<spdlog::logger> s_CoreLogger;
  static std::shared_ptr<spdlog::logger> s_ClientLogger;
};

} // namespace Elysium

// ---------------------------------------------------------------------------
// Engine / Core logging macros
// ---------------------------------------------------------------------------
#ifndef ELYSIUM_DIST
#define ELYSIUM_CORE_TRACE(...)                                                \
  ::Elysium::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ELYSIUM_CORE_INFO(...)                                                 \
  ::Elysium::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ELYSIUM_CORE_WARN(...)                                                 \
  ::Elysium::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ELYSIUM_CORE_ERROR(...)                                                \
  ::Elysium::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ELYSIUM_CORE_CRITICAL(...)                                             \
  ::Elysium::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client logging macros
#define ELYSIUM_TRACE(...) ::Elysium::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ELYSIUM_INFO(...) ::Elysium::Log::GetClientLogger()->info(__VA_ARGS__)
#define ELYSIUM_WARN(...) ::Elysium::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ELYSIUM_ERROR(...) ::Elysium::Log::GetClientLogger()->error(__VA_ARGS__)
#define ELYSIUM_CRITICAL(...)                                                  \
  ::Elysium::Log::GetClientLogger()->critical(__VA_ARGS__)
#else
  // Distribution build: strip everything to no-ops
#define ELYSIUM_CORE_TRACE(...)
#define ELYSIUM_CORE_INFO(...)
#define ELYSIUM_CORE_WARN(...)
#define ELYSIUM_CORE_ERROR(...)
#define ELYSIUM_CORE_CRITICAL(...)

#define ELYSIUM_TRACE(...)
#define ELYSIUM_INFO(...)
#define ELYSIUM_WARN(...)
#define ELYSIUM_ERROR(...)
#define ELYSIUM_CRITICAL(...)
#endif