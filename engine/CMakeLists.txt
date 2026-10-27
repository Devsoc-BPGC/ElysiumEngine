set(ENGINE_SOURCES
    physics/src/AABB.cpp
    physics/src/Collider.cpp
    physics/src/PhysicsWorld.cpp
    physics/src/RigidBody.cpp
)

# Core src seems empty for now, but we'll add it if there are any files
file(GLOB_RECURSE CORE_SOURCES core/src/*.cpp)
list(APPEND ENGINE_SOURCES ${CORE_SOURCES})

# Editor sources
file(GLOB_RECURSE EDITOR_SOURCES editor/src/*.cpp)
list(APPEND ENGINE_SOURCES ${EDITOR_SOURCES})

# Scripting sources
file(GLOB_RECURSE SCRIPTING_SOURCES scripting/src/*.cpp)
list(APPEND ENGINE_SOURCES ${SCRIPTING_SOURCES})

# Changed from SHARED to STATIC to avoid DLL export issues on Windows
add_library(ElysiumEngine STATIC ${ENGINE_SOURCES})

target_include_directories(ElysiumEngine PUBLIC
    core/include
    physics/include
    editor/include
    scripting/include
    events
)

if(WIN32)
    target_compile_definitions(ElysiumEngine PUBLIC ELYSIUM_PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(ElysiumEngine PUBLIC ELYSIUM_PLATFORM_MAC)
elseif(UNIX)
    target_compile_definitions(ElysiumEngine PUBLIC ELYSIUM_PLATFORM_LINUX)
endif()


target_link_libraries(ElysiumEngine PUBLIC
    SFML::Graphics 
    SFML::Audio
    SFML::Window 
    SFML::System 
    spdlog::spdlog_header_only
    ImGui-SFML::ImGui-SFML
)
