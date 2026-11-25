# ==============================================================================
#  Unix (Linux / macOS) LIBDIR & SDK resolution
# ==============================================================================

if(APPLE)
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(_elysium_platform_dir "macos_arm64")
    else()
        set(_elysium_platform_dir "macos_x64")
    endif()
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(_elysium_platform_dir "linux_arm64")
    else()
        set(_elysium_platform_dir "linux_x64")
    endif()
endif()

set(_elysium_default_libdir "${CMAKE_SOURCE_DIR}/../lib/${_elysium_platform_dir}")
if(EXISTS "${_elysium_default_libdir}")
    set(ELYSIUM_LIBDIR "${_elysium_default_libdir}" CACHE PATH "Path to prebuilt Elysium SDKs (Blender-style LIBDIR)")
else()
    set(ELYSIUM_LIBDIR "" CACHE PATH "Path to prebuilt Elysium SDKs (Blender-style LIBDIR)")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/platform_dependencies.cmake")
