/**
 * @file Core.hpp
 * @brief Platform-specific macros and API export/import definitions.
 */

#pragma once

// For static library, ELYSIUM_API is empty
#define ELYSIUM_API

// Platform detection (now defined by CMake, but fallback here)
#if defined(ELYSIUM_PLATFORM_WINDOWS) || defined(_WIN32)
#undef ELYSIUM_PLATFORM_WINDOWS
#undef ELYSIUM_PLATFORM_LINUX
#undef ELYSIUM_PLATFORM_MAC
#define ELYSIUM_PLATFORM_WINDOWS 1
#elif defined(ELYSIUM_PLATFORM_MAC) || defined(__APPLE__)
#undef ELYSIUM_PLATFORM_WINDOWS
#undef ELYSIUM_PLATFORM_LINUX
#undef ELYSIUM_PLATFORM_MAC
#define ELYSIUM_PLATFORM_MAC 1
#elif defined(ELYSIUM_PLATFORM_LINUX) || defined(__linux__)
#undef ELYSIUM_PLATFORM_WINDOWS
#undef ELYSIUM_PLATFORM_LINUX
#undef ELYSIUM_PLATFORM_MAC
#define ELYSIUM_PLATFORM_LINUX 1
#endif

