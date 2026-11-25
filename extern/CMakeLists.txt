# ==============================================================================
#  Vendored source dependencies (Dear ImGui & ImGui-SFML)
# ==============================================================================

add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-sfml"
)
target_compile_definitions(imgui PUBLIC
    IMGUI_USER_CONFIG="imconfig-SFML.h"
)
target_link_libraries(imgui PUBLIC
    SFML::Graphics
    SFML::System
)

add_library(ImGui-SFML STATIC
    imgui-sfml/imgui-SFML.cpp
)
add_library(ImGui-SFML::ImGui-SFML ALIAS ImGui-SFML)

target_include_directories(ImGui-SFML PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui-sfml"
)
target_link_libraries(ImGui-SFML PUBLIC
    imgui
    SFML::Graphics
    SFML::Window
    SFML::System
)

if(APPLE)
    target_link_libraries(ImGui-SFML PUBLIC "-framework OpenGL")
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(ImGui-SFML PUBLIC OpenGL::GL)
    if(WIN32 AND MINGW)
        target_link_libraries(ImGui-SFML PUBLIC imm32)
    endif()
endif()
