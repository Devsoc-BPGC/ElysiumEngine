#ifndef EDITORCONTEXT_HPP
#define EDITORCONTEXT_HPP

#include "CommandHistory.hpp"
#include "GameObject.hpp"
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <memory>
#include <string>

class Scene;

namespace Elysium {

enum class ScenePlayState { Edit, Play, Pause, Step };

enum class GizmoOperation {
  None,
  Translate,
  Rotate,
  Scale
};

enum class GizmoMode {
  Local,
  World
};

struct EditorCamera {
  sf::Vector2f center = {8.0f, 6.0f}; // Default center in meters
  float zoom = 1.0f;                  // 1.0f = 100% zoom
  bool isPanning = false;

  void Reset() {
    center = {8.0f, 6.0f};
    zoom = 1.0f;
  }

  /**
   * @brief Converts pixel offset from the viewport center to world coordinates in meters.
   * @param viewportPixelOffset Pixel offset relative to center of the viewport (x: -w/2..w/2, y: -h/2..h/2).
   * @param pixelsPerMeter Conversion factor from meters to pixels.
   */
  sf::Vector2f ScreenToWorld(const sf::Vector2f &viewportPixelOffset, float pixelsPerMeter) const {
    float effectivePtm = pixelsPerMeter * zoom;
    if (effectivePtm <= 0.0f) effectivePtm = 1.0f;
    return {
      center.x + viewportPixelOffset.x / effectivePtm,
      center.y + viewportPixelOffset.y / effectivePtm
    };
  }

  /**
   * @brief Converts mouse position within the viewport image to world coordinates in meters.
   * @param mousePosInViewport Coordinates relative to the top-left of the viewport image [0..w, 0..h].
   * @param viewportSize Dimensions of the viewport image in pixels.
   * @param pixelsPerMeter Conversion factor from meters to pixels.
   */
  sf::Vector2f ViewportToWorld(const sf::Vector2f &mousePosInViewport,
                               const sf::Vector2f &viewportSize,
                               float pixelsPerMeter) const {
    sf::Vector2f pixelFromCenter = {
      mousePosInViewport.x - viewportSize.x * 0.5f,
      mousePosInViewport.y - viewportSize.y * 0.5f
    };
    return ScreenToWorld(pixelFromCenter, pixelsPerMeter);
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
  CommandHistory history;

  void SelectEntity(const std::shared_ptr<GameObject> &entity) { selectedEntity = entity; }
  void ClearSelection() { selectedEntity = nullptr; }

  // Copy of the scene being simulated while not in Edit mode (null in Edit
  // mode). Play never touches the edit scene shown in the scene viewport.
  Scene *runtimeScene = nullptr;

  // Visual settings
  bool drawDebugGrid = true;
  bool drawPhysicsColliders = true;
  bool drawAABBs = false;
  bool gamePreviewDebugVisuals = false; // Editor-only visuals in Game Preview
  float pixelsPerMeter = 50.0f;

  // Gizmo & Transformation settings
  GizmoOperation gizmoOperation = GizmoOperation::Translate;
  GizmoMode gizmoMode = GizmoMode::World;

  // Snapping settings
  bool snapEnabled = false;
  float snapTranslation = 0.5f; // Grid snap step in meters
  float snapRotation = 15.0f;   // Angle snap step in degrees
  float snapScale = 0.1f;       // Scale snap step

  // Viewport geometry (in pixels)
  sf::Vector2f viewportPos = {0.0f, 0.0f};
  sf::Vector2f viewportSize = {800.0f, 600.0f};
  bool isViewportHovered = false;
  bool isViewportFocused = false;
  unsigned int viewportDockId = 0; // ImGuiID of its dock node; 0 if floating
};

} // namespace Elysium

#endif // EDITORCONTEXT_HPP
