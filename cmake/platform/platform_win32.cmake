# ==============================================================================
#  Windows LIBDIR & SDK resolution
# ==============================================================================

set(_elysium_default_libdir "${CMAKE_SOURCE_DIR}/../lib/windows_x64")
if(EXISTS "${_elysium_default_libdir}")
    set(ELYSIUM_LIBDIR "${_elysium_default_libdir}" CACHE PATH "Path to prebuilt Elysium SDKs (Blender-style LIBDIR)")
else()
    set(ELYSIUM_LIBDIR "" CACHE PATH "Path to prebuilt Elysium SDKs (Blender-style LIBDIR)")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/platform_dependencies.cmake")
