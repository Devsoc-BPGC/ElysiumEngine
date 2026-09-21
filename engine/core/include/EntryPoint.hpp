/**
 * @file EntryPoint.hpp
 * @brief The platform-specific entry point (main) for the engine.
 */

#pragma once

#include "Application.hpp"
#include "Log.h"

#ifdef ELYSIUM_PLATFORM_LINUX

/**
 * @brief External declaration of the client factory function.
 */
extern Elysium::Application *Elysium::CreateApplication();

/**
 * @brief The main function that bootstraps the engine and client application.
 * @param argc Number of command line arguments.
 * @param argv Array of command line argument strings.
 * @return Exit status code.
 */
int main(int argc, char **argv) {
  Elysium::Log::Init();
  auto app = Elysium::CreateApplication();
  app->Run();
  delete app;
  return 0;
}

#endif
