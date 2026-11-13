#ifndef EDITORHOTKEYS_HPP
#define EDITORHOTKEYS_HPP

#include <SFML/Window/Keyboard.hpp>

namespace Elysium {
namespace Hotkeys {

// Playback / Simulation
constexpr auto TogglePlayPause       = sf::Keyboard::Key::Space;

// History / Undo & Redo
constexpr auto Undo                  = sf::Keyboard::Key::Z;
constexpr auto Redo                  = sf::Keyboard::Key::Y;

// Entity Navigation & Modification
constexpr auto FocusCamera           = sf::Keyboard::Key::F;
constexpr auto DeleteEntity          = sf::Keyboard::Key::Delete;

// Transform Gizmo Operations (Blender-style)
constexpr auto GizmoTranslate        = sf::Keyboard::Key::G; // Move / Grab
constexpr auto GizmoRotate           = sf::Keyboard::Key::R; // Rotate
constexpr auto GizmoScale            = sf::Keyboard::Key::S; // Scale

// Gizmo Modifiers & Toggles
constexpr auto GizmoToggleMode       = sf::Keyboard::Key::W; // Toggle World / Local coordinate space
constexpr auto GizmoToggleSnap       = sf::Keyboard::Key::E; // Toggle Snapping on / off

} // namespace Hotkeys
} // namespace Elysium

#endif // EDITORHOTKEYS_HPP
