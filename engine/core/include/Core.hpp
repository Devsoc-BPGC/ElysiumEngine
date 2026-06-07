/**
 * @file Core.hpp
 * @brief Platform-specific macros and API export/import definitions.
 */

#pragma once

// For static library, ELYSIUM_API is empty
#define ELYSIUM_API

// Platform detection (now defined by CMake, but fallback here)
#ifndef ELYSIUM_PLATFORM_WINDOWS
    #ifdef _WIN32
        #define ELYSIUM_PLATFORM_WINDOWS 1
    #endif
#endif

#ifndef ELYSIUM_PLATFORM_LINUX
    #ifdef __linux__
        #define ELYSIUM_PLATFORM_LINUX 1
    #endif
#endif

#ifndef ELYSIUM_PLATFORM_MAC
    #ifdef __APPLE__
        #define ELYSIUM_PLATFORM_MAC 1
    #endif
#endif
