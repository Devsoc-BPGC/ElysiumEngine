#pragma once

#include "KeyCodes.hpp"
#include "MouseCodes.hpp"
#include "ScriptTypes.hpp"
#include <string>

namespace Elysium {

/**
 * @brief Checks if a key is held down.
 * @param key Key code.
 * @return true if held down, false otherwise.
 */
bool IsKeyDown(KeyCode key);

/**
 * @brief Checks if a key is held down.
 * @param keyName Key name string.
 * @return true if held down, false otherwise.
 */
bool IsKeyDown(const std::string& keyName);

/**
 * @brief Checks if a key was pressed this frame.
 * @param key Key code.
 * @return true if pressed this frame, false otherwise.
 */
bool IsKeyPressed(KeyCode key);

/**
 * @brief Checks if a key was pressed this frame.
 * @param keyName Key name string.
 * @return true if pressed this frame, false otherwise.
 */
bool IsKeyPressed(const std::string& keyName);

/**
 * @brief Checks if a key was released this frame.
 * @param key Key code.
 * @return true if released this frame, false otherwise.
 */
bool IsKeyReleased(KeyCode key);

/**
 * @brief Checks if a key was released this frame.
 * @param keyName Key name string.
 * @return true if released this frame, false otherwise.
 */
bool IsKeyReleased(const std::string& keyName);

/**
 * @brief Checks if a mouse button is held down.
 * @param button Mouse button code.
 * @return true if held down, false otherwise.
 */
bool IsMouseDown(MouseCode button);

/**
 * @brief Checks if a mouse button was clicked this frame.
 * @param button Mouse button code.
 * @return true if clicked this frame, false otherwise.
 */
bool IsMouseClicked(MouseCode button);

/**
 * @brief Gets the mouse position in world coordinates.
 * @return Mouse position as Vector2.
 */
Vector2 GetMousePosition();

} // namespace Elysium
