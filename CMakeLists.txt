cmake_minimum_required(VERSION 3.20)

# --- Project ---
project(ElysiumEngineProject
    VERSION 1.0.0
    DESCRIPTION "Elysium game engine"
    LANGUAGES CXX
)

# --- C++ standard ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Export compile_commands.json for clangd/LSP ---
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Default build type ---
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

# --- Test toggle (must be declared before enable_testing / add_test) ---
option(BUILD_TESTING "Build tests" OFF)

# --- Distribution build toggle (strips all logging) ---
option(ELYSIUM_DIST "Build in distribution mode (no logging)" OFF)

# ============================================================
#  Helper: configure a target with platform-specific settings
# ============================================================
function(configure_elysium_target target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wshadow -Wnon-virtual-dtor -Wcast-align
            -Wunused -Woverloaded-virtual -Wconversion
            -Wsign-conversion -Wdouble-promotion
        )

        if(APPLE)
            target_compile_options(${target_name} PRIVATE
                -mmacosx-version-min=10.15)
        elseif(UNIX)
            target_compile_options(${target_name} PRIVATE -D_GNU_SOURCE)
        endif()
    endif()

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
endfunction()

# ============================================================
#  Platform defines
# ============================================================
if(WIN32)
    add_compile_definitions(ELYSIUM_PLATFORM_WINDOWS)
elseif(APPLE)
    add_compile_definitions(ELYSIUM_PLATFORM_MAC)
elseif(UNIX)
    add_compile_definitions(ELYSIUM_PLATFORM_LINUX)
endif()

# ============================================================
#  Debug/Release flags for GCC/Clang
# ============================================================
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
endif()

# ============================================================
#  spdlog configuration
# ============================================================
# Header-only mode is the ONLY reliable way to use spdlog here.
# It avoids every ABI/symbol-mismatch issue with the compiled lib.
set(SPDLOG_FMT_EXTERNAL     OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL          OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED     OFF CACHE BOOL "" FORCE)

add_subdirectory(external/spdlog)

# ============================================================
#  SFML configuration
# ============================================================
set(SFML_BUILD_EXAMPLES      OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_PROGRAMS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC           OFF CACHE BOOL "" FORCE)
set(SFML_INSTALL             OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK       OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO         OFF CACHE BOOL "" FORCE)

add_subdirectory(external/SFML)

# ============================================================
#  Engine library
# ============================================================
add_subdirectory(engine)

if(ELYSIUM_DIST)
    target_compile_definitions(ElysiumEngine PUBLIC ELYSIUM_DIST)
    message(STATUS "ELYSIUM_DIST is ON — all logging will be compiled out")
endif()

# ============================================================
#  Application
# ============================================================
add_executable(ElysiumApp app/main.cpp)
target_link_libraries(ElysiumApp PUBLIC
    ElysiumEngine
    SFML::Graphics
    SFML::Window
    SFML::System
    spdlog::spdlog_header_only
)
configure_elysium_target(ElysiumApp)

# Windows: copy runtime DLLs next to the app (spdlog removed — header-only)
if(WIN32)
    add_custom_command(TARGET ElysiumApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::Graphics>
            $<TARGET_FILE:SFML::Window>
            $<TARGET_FILE:SFML::System>
            $<TARGET_FILE_DIR:ElysiumApp>
        COMMENT "Copying runtime DLLs next to ElysiumApp"
        VERBATIM
    )
endif()

# ============================================================
#  Tests (opt-in via -DBUILD_TESTING=ON)
# ============================================================
if(BUILD_TESTING)
    enable_testing()

    function(elysium_add_test test_name source_file)
        add_executable(${test_name} ${source_file})
        target_link_libraries(${test_name} PRIVATE
            ElysiumEngine
            SFML::System
            spdlog::spdlog_header_only
        )
        configure_elysium_target(${test_name})
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    elysium_add_test(PhysicsPerformanceTest   test/PhysicsPerformanceTest.cpp)
    elysium_add_test(BroadPhasePerformanceTest test/BroadPhasePerformanceTest.cpp)
    elysium_add_test(BoundaryTest              test/BoundaryTest.cpp)
    elysium_add_test(FrictionTest              test/FrictionTest.cpp)
endif()

# ============================================================
#  Custom targets
# ============================================================
find_program(CLANG_FORMAT_EXE clang-format)

if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ELYSIUM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/engine/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/app/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ELYSIUM_SOURCES}
        COMMENT "Formatting C++ sources with clang-format"
        VERBATIM
    )
else()
    add_custom_target(format
        COMMAND ${CMAKE_COMMAND} -E echo
            "clang-format not found — install it to use 'make format'"
    )
endif()

add_custom_target(run
    COMMAND $<TARGET_FILE:ElysiumApp>
    DEPENDS ElysiumApp
    COMMENT "Running ElysiumApp"
    USES_TERMINAL
)

# ============================================================
#  Status output
# ============================================================
message(STATUS "")
message(STATUS "===== Elysium configuration =====")
message(STATUS "Build type     : ${CMAKE_BUILD_TYPE}")
message(STATUS "System         : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler       : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Source dir     : ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Binary dir     : ${CMAKE_BINARY_DIR}")
message(STATUS "Output bin dir : ${CMAKE_BINARY_DIR}/bin")
message(STATUS "Tests enabled  : ${BUILD_TESTING}")
message(STATUS "Dist build     : ${ELYSIUM_DIST}")
if(WIN32)
    message(STATUS "Platform defs  : ELYSIUM_PLATFORM_WINDOWS")
elseif(APPLE)
    message(STATUS "Platform defs  : ELYSIUM_PLATFORM_MAC")
elseif(UNIX)
    message(STATUS "Platform defs  : ELYSIUM_PLATFORM_LINUX")
endif()
message(STATUS "=================================")
message(STATUS "")