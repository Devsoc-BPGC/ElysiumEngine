#pragma once

// Platform-specific defines to prevent macro pollution on Windows/MSVC
#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
#endif

// ---------------------------------------------------------------------------
// Standard C++20 Library Headers
// ---------------------------------------------------------------------------
// Containers & Utilities
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// Streams & Filesystem
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

// Math, Time & Concurrency
#include <chrono>
#include <cmath>
#include <mutex>
#include <random>

// ---------------------------------------------------------------------------
// Core Third-Party & Stable Internal Utilities
// ---------------------------------------------------------------------------
#include <spdlog/spdlog.h>
#include "Log.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include <imgui.h>
