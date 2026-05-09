/**
 * @file Core.hpp
 * @brief Platform-specific macros and API export/import definitions.
 */

#pragma once

#ifdef ELYSIUM_PLATFORM_WINDOWS
    #ifdef ELYSIUM_BUILD_DLL
        #define ELYSIUM_API __declspec(dllexport)
    #else
        #define ELYSIUM_API __declspec(dllimport)
    #endif
#else
    #define ELYSIUM_API __attribute__((visibility("default")))
#endif
