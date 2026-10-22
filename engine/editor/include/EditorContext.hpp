#ifndef EDITORCONTEXT_HPP
#define EDITORCONTEXT_HPP

#include "GameObject.hpp"
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace Elysium {

enum class ScenePlayState { Edit, Play, Pause, Step };

struct EditorCamera {
  sf::Vector2f center = {8.0f, 6.0f}; // Default center in meters
  float zoom = 1.0f;                  // 1.0f = 100% zoom
  bool isPanning = false;

  void Reset() {
    center = {8.0f, 6.0f};
    zoom = 1.0f;
  }
};

// Drives the Game Preview. Kept separate from EditorCamera so navigating one
// view never moves the other.
struct GameCamera {
  sf::Vector2f center = {8.0f, 6.0f}; // Framing center in meters
  float viewHeight = 15.0f;           // World meters visible vertically
  float aspectRatio = 16.0f / 9.0f;   // Output aspect; <= 0 fills the panel

  void Reset() {
    center = {8.0f, 6.0f};
    viewHeight = 15.0f;
    aspectRatio = 16.0f / 9.0f;
  }
};

struct EditorStats {
  float fps = 0.0f;
  float frameTimeMs = 0.0f;
  float updateTimeMs = 0.0f;
  float physicsTimeMs = 0.0f;
  float renderTimeMs = 0.0f;
  uint32_t entityCount = 0;
  uint32_t drawCalls = 0;
};

struct EditorContext {
  ScenePlayState playState = ScenePlayState::Edit;
  std::shared_ptr<GameObject> selectedEntity = nullptr;
  EditorCamera camera;
  GameCamera gameCamera;
  EditorStats stats;

  // Visual settings
  bool drawDebugGrid = true;
  bool drawPhysicsColliders = true;
  bool drawAABBs = false;
  bool gamePreviewDebugVisuals = false; // Editor-only visuals in Game Preview
  float pixelsPerMeter = 50.0f;

  // Viewport geometry (in pixels)
  sf::Vector2f viewportPos = {0.0f, 0.0f};
  sf::Vector2f viewportSize = {800.0f, 600.0f};
  bool isViewportHovered = false;
  bool isViewportFocused = false;
};

} // namespace Elysium

#endif // EDITORCONTEXT_HPP
