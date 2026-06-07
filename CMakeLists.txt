cmake_minimum_required(VERSION 3.15)
project(ElysiumEngineProject VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection for compiler definitions
if(WIN32)
  add_compile_definitions(ELYSIUM_PLATFORM_WINDOWS)
elseif(APPLE)
  add_compile_definitions(ELYSIUM_PLATFORM_MAC)
elseif(UNIX)
  add_compile_definitions(ELYSIUM_PLATFORM_LINUX)
endif()

# SFML Configuration
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_PROGRAMS OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_INSTALL OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)  # Disable network to avoid deps
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)    # Disable audio to avoid deps

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add SFML from external directory
add_subdirectory(external/SFML)

add_subdirectory(external/glfw)

# Add engine subdirectory
add_subdirectory(engine)

# Add application
add_executable(ElysiumApp app/main.cpp)

# Link application with engine and SFML
# Remove STATIC keyword - it doesn't belong here
target_link_libraries(ElysiumApp PRIVATE ElysiumEngine SFML::Graphics SFML::Window SFML::System glfw)

# Set include directories for the app to find engine headers
target_include_directories(ElysiumApp PRIVATE
    engine/core/include
    engine/physics/include
)

# Add Performance Tests
add_executable(PhysicsPerformanceTest test/PhysicsPerformanceTest.cpp)
target_link_libraries(PhysicsPerformanceTest PRIVATE ElysiumEngine SFML::System)
target_include_directories(PhysicsPerformanceTest PRIVATE
    engine/core/include
    engine/physics/include
)

add_executable(BroadPhasePerformanceTest test/BroadPhasePerformanceTest.cpp)
target_link_libraries(BroadPhasePerformanceTest PRIVATE ElysiumEngine SFML::System)
target_include_directories(BroadPhasePerformanceTest PRIVATE
    engine/core/include
    engine/physics/include
)

add_executable(BoundaryTest test/BoundaryTest.cpp)
target_link_libraries(BoundaryTest PRIVATE ElysiumEngine SFML::System)
target_include_directories(BoundaryTest PRIVATE
    engine/core/include
    engine/physics/include
)

add_executable(FrictionTest test/FrictionTest.cpp)
target_link_libraries(FrictionTest PRIVATE ElysiumEngine SFML::System)
target_include_directories(FrictionTest PRIVATE
    engine/core/include
    engine/physics/include
)

if(WIN32)
  add_custom_command(TARGET ElysiumApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:glfw>
        $<TARGET_FILE:SFML::Graphics>
        $<TARGET_FILE:SFML::Window>
        $<TARGET_FILE:SFML::System>
        $<TARGET_FILE_DIR:ElysiumApp>
    )
endif()

enable_testing()
add_test(NAME PhysicsPerformanceTest COMMAND PhysicsPerformanceTest)
add_test(NAME BroadPhasePerformanceTest COMMAND BroadPhasePerformanceTest)
add_test(NAME BoundaryTest COMMAND BoundaryTest)
add_test(NAME FrictionTest COMMAND FrictionTest)
