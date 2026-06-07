set(ENGINE_SOURCES
    physics/src/AABB.cpp
    physics/src/Collider.cpp
    physics/src/PhysicsWorld.cpp
    physics/src/RigidBody.cpp
)

# Core src seems empty for now, but we'll add it if there are any files
file(GLOB_RECURSE CORE_SOURCES core/src/*.cpp)
list(APPEND ENGINE_SOURCES ${CORE_SOURCES})

# Changed from SHARED to STATIC to avoid DLL export issues on Windows
add_library(ElysiumEngine STATIC ${ENGINE_SOURCES})

target_include_directories(ElysiumEngine PUBLIC
    core/include
    physics/include
)

# Remove ELYSIUM_BUILD_DLL since we're building static
# Let CMake handle platform detection instead of hardcoding
target_compile_definitions(ElysiumEngine PUBLIC
    $<$<BOOL:${WIN32}>:ELYSIUM_PLATFORM_WINDOWS>
    $<$<BOOL:${APPLE}>:ELYSIUM_PLATFORM_MAC>
    $<$<BOOL:${UNIX} AND NOT ${APPLE}>:ELYSIUM_PLATFORM_LINUX>
)

target_link_libraries(ElysiumEngine PUBLIC SFML::Graphics SFML::Window SFML::System)
