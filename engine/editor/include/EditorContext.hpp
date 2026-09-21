#ifndef EDITORCONTEXT_HPP
#define EDITORCONTEXT_HPP

#include "GameObject.hpp"
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <memory>
#include <string>

namespace Elysium {

enum class ScenePlayState {
  Edit,
  Play,
  Pause,
  Step
};

struct EditorCamera {
  sf::Vector2f center = {8.0f, 6.0f}; // Default center in meters
  float zoom = 1.0f;                  // 1.0f = 100% zoom
  bool isPanning = false;

  void Reset() {
    center = {8.0f, 6.0f};
    zoom = 1.0f;
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
  EditorStats stats;

  // Visual settings
  bool drawDebugGrid = true;
  bool drawPhysicsColliders = true;
  bool drawAABBs = false;
  float pixelsPerMeter = 50.0f;

  // Viewport geometry (in pixels)
  sf::Vector2f viewportPos = {0.0f, 0.0f};
  sf::Vector2f viewportSize = {800.0f, 600.0f};
  bool isViewportHovered = false;
  bool isViewportFocused = false;
};

} // namespace Elysium

#endif // EDITORCONTEXT_HPP
