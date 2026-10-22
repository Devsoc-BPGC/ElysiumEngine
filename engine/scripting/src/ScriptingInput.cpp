#include "ScriptingInput.hpp"
#include "Input.hpp"
#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace Elysium {

/**
 * @brief Converts a key name string to a KeyCode.
 * @param name Key name string.
 * @return KeyCode enum value.
 */
static KeyCode StringToKeyCode(std::string name) {
    for (char& c : name) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    static const std::unordered_map<std::string, KeyCode> s_KeyMap = {
        {"A", KeyCode::A}, {"B", KeyCode::B}, {"C", KeyCode::C}, {"D", KeyCode::D},
        {"E", KeyCode::E}, {"F", KeyCode::F}, {"G", KeyCode::G}, {"H", KeyCode::H},
        {"I", KeyCode::I}, {"J", KeyCode::J}, {"K", KeyCode::K}, {"L", KeyCode::L},
        {"M", KeyCode::M}, {"N", KeyCode::N}, {"O", KeyCode::O}, {"P", KeyCode::P},
        {"Q", KeyCode::Q}, {"R", KeyCode::R}, {"S", KeyCode::S}, {"T", KeyCode::T},
        {"U", KeyCode::U}, {"V", KeyCode::V}, {"W", KeyCode::W}, {"X", KeyCode::X},
        {"Y", KeyCode::Y}, {"Z", KeyCode::Z},
        {"SPACE", KeyCode::Space}, {"ENTER", KeyCode::Enter}, {"RETURN", KeyCode::Enter},
        {"ESCAPE", KeyCode::Escape}, {"ESC", KeyCode::Escape},
        {"LSHIFT", KeyCode::LShift}, {"SHIFT", KeyCode::LShift}, {"RSHIFT", KeyCode::RShift},
        {"LCTRL", KeyCode::LControl}, {"CTRL", KeyCode::LControl}, {"RCTRL", KeyCode::RControl},
        {"TAB", KeyCode::Tab}, {"BACKSPACE", KeyCode::Backspace},
        {"LEFT", KeyCode::Left}, {"RIGHT", KeyCode::Right}, {"UP", KeyCode::Up}, {"DOWN", KeyCode::Down},
        {"0", KeyCode::Num0}, {"1", KeyCode::Num1}, {"2", KeyCode::Num2}, {"3", KeyCode::Num3},
        {"4", KeyCode::Num4}, {"5", KeyCode::Num5}, {"6", KeyCode::Num6}, {"7", KeyCode::Num7},
        {"8", KeyCode::Num8}, {"9", KeyCode::Num9}
    };

    auto it = s_KeyMap.find(name);
    return (it != s_KeyMap.end()) ? it->second : KeyCode::Unknown;
}

/**
 * @brief Checks if a key is held down.
 * @param key Key code.
 * @return true if held down, false otherwise.
 */
bool IsKeyDown(KeyCode key) {
    return Input::IsKeyDown(key);
}

/**
 * @brief Checks if a key is held down.
 * @param keyName Key name string.
 * @return true if held down, false otherwise.
 */
bool IsKeyDown(const std::string& keyName) {
    return IsKeyDown(StringToKeyCode(keyName));
}

/**
 * @brief Checks if a key was pressed this frame.
 * @param key Key code.
 * @return true if pressed this frame, false otherwise.
 */
bool IsKeyPressed(KeyCode key) {
    return Input::IsKeyPressed(key);
}

/**
 * @brief Checks if a key was pressed this frame.
 * @param keyName Key name string.
 * @return true if pressed this frame, false otherwise.
 */
bool IsKeyPressed(const std::string& keyName) {
    return IsKeyPressed(StringToKeyCode(keyName));
}

/**
 * @brief Checks if a key was released this frame.
 * @param key Key code.
 * @return true if released this frame, false otherwise.
 */
bool IsKeyReleased(KeyCode key) {
    return Input::IsKeyReleased(key);
}

/**
 * @brief Checks if a key was released this frame.
 * @param keyName Key name string.
 * @return true if released this frame, false otherwise.
 */
bool IsKeyReleased(const std::string& keyName) {
    return IsKeyReleased(StringToKeyCode(keyName));
}

/**
 * @brief Checks if a mouse button is held down.
 * @param button Mouse button code.
 * @return true if held down, false otherwise.
 */
bool IsMouseDown(MouseCode button) {
    return Input::IsMouseButtonDown(button);
}

/**
 * @brief Checks if a mouse button was clicked this frame.
 * @param button Mouse button code.
 * @return true if clicked this frame, false otherwise.
 */
bool IsMouseClicked(MouseCode button) {
    return Input::IsMouseButtonPressed(button);
}

/**
 * @brief Gets the mouse position in world coordinates.
 * @return Mouse position as Vector2.
 */
Vector2 GetMousePosition() {
    sf::Vector2f rawPos = Input::GetMousePosition();
    constexpr float PIXELS_PER_METER = 50.0f;
    return {rawPos.x / PIXELS_PER_METER, rawPos.y / PIXELS_PER_METER};
}

} // namespace Elysium
