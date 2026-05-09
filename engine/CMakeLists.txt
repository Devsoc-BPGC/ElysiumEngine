set(ENGINE_SOURCES
    physics/src/AABB.cpp
    physics/src/Collider.cpp
    physics/src/PhysicsWorld.cpp
    physics/src/RigidBody.cpp
)

# Core src seems empty for now, but we'll add it if there are any files
file(GLOB_RECURSE CORE_SOURCES core/src/*.cpp)
list(APPEND ENGINE_SOURCES ${CORE_SOURCES})

add_library(ElysiumEngine SHARED ${ENGINE_SOURCES})

target_include_directories(ElysiumEngine PUBLIC 
    core/include
    physics/include
)

target_compile_definitions(ElysiumEngine PRIVATE ELYSIUM_BUILD_DLL)
target_compile_definitions(ElysiumEngine PUBLIC ELYSIUM_PLATFORM_LINUX)

target_link_libraries(ElysiumEngine PUBLIC SFML::Graphics SFML::Window SFML::System)
