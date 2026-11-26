/**
 * @file EntryPoint.hpp
 * @brief The platform-specific entry point (main) for the engine.
 */

#pragma once

#include "Application.hpp"
#include "Log.h"

#if defined(ELYSIUM_PLATFORM_WINDOWS) || defined(ELYSIUM_PLATFORM_LINUX) || defined(ELYSIUM_PLATFORM_MAC) || defined(ELYSIUM_PLATFORM_MACOS) || defined(_WIN32) || defined(__linux__) || defined(__APPLE__)

/**
 * @brief External declaration of the client factory function.
 */
extern Elysium::Application *
Elysium::CreateApplication(Elysium::ApplicationCommandLineArgs args);

/**
 * @brief The main function that bootstraps the engine and client application.
 * @param argc Number of command line arguments.
 * @param argv Array of command line argument strings.
 * @return Exit status code.
 */
int main(int argc, char **argv) {
  Elysium::Log::Init();
  auto app = Elysium::CreateApplication({argc, argv});
  app->Run();
  delete app;
  return 0;
}

#endif

