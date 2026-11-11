#ifndef TRANSFORMGIZMO_HPP
#define TRANSFORMGIZMO_HPP

#include "CoreMath.hpp"
#include "EditorContext.hpp"
#include "GameObject.hpp"
#include <imgui.h>

namespace Elysium {

enum class GizmoHandle {
  None,
  TranslateX,
  TranslateY,
  TranslateXY,
  RotateRing,
  ScaleX,
  ScaleY,
  ScaleXY
};

class TransformGizmo {
public:
  TransformGizmo() = default;

  /**
   * @brief Renders the active transform gizmo and handles mouse dragging.
   * @param context Editor context containing camera, selected entity, and gizmo modes.
   * @param activeScene Active scene owning the entities.
   * @param imageMin Screen coordinates of top-left corner of the viewport image.
   * @param imageSize Dimensions of the viewport image in pixels.
   * @param drawList ImGui draw list (window draw list).
   */
  void Render(EditorContext &context, Scene &activeScene,
              const ImVec2 &imageMin, const ImVec2 &imageSize,
              ImDrawList *drawList);

  /**
   * @brief Returns true if mouse cursor is currently hovering over any gizmo handle.
   */
  bool IsOver() const { return m_hoveredHandle != GizmoHandle::None; }

  /**
   * @brief Returns true if the user is currently actively dragging a gizmo handle.
   */
  bool IsUsing() const { return m_activeHandle != GizmoHandle::None; }

  /**
   * @brief Resets gizmo state (e.g. when selection changes or interaction is cancelled).
   */
  void Reset();

private:
  GizmoHandle m_hoveredHandle = GizmoHandle::None;
  GizmoHandle m_activeHandle = GizmoHandle::None;
  uint64_t m_activeEntityUUID = 0;

  // Stored state at the start of a drag
  ImVec2 m_dragStartMouse = ImVec2(0.0f, 0.0f);
  Vec3 m_initialWorldPos = Vec3(0.0f, 0.0f, 0.0f);
  Quat m_initialWorldRot = Quat();
  Vec3 m_initialWorldScale = Vec3(1.0f, 1.0f, 1.0f);
  Vec3 m_initialLocalPos = Vec3(0.0f, 0.0f, 0.0f);
  Quat m_initialLocalRot = Quat();
  Vec3 m_initialLocalScale = Vec3(1.0f, 1.0f, 1.0f);

  // Rotation drag tracking
  float m_initialAngleRad = 0.0f;
  float m_currentAngleDeltaRad = 0.0f;

  ImVec2 WorldToScreen(const Vec3 &worldPos, const EditorContext &context,
                       const ImVec2 &imageMin, const ImVec2 &imageSize) const;

  void RenderTranslateGizmo(EditorContext &context, GameObject &entity,
                            const ImVec2 &originScreen, const Vec2 &axisX, const Vec2 &axisY,
                            ImDrawList *drawList, const ImVec2 &mousePos,
                            bool isMouseDown, bool isMouseClicked);

  void RenderRotateGizmo(EditorContext &context, GameObject &entity,
                         const ImVec2 &originScreen,
                         ImDrawList *drawList, const ImVec2 &mousePos,
                         bool isMouseDown, bool isMouseClicked);

  void RenderScaleGizmo(EditorContext &context, GameObject &entity,
                        const ImVec2 &originScreen, const Vec2 &axisX, const Vec2 &axisY,
                        ImDrawList *drawList, const ImVec2 &mousePos,
                        bool isMouseDown, bool isMouseClicked);

  void ApplyWorldTransform(GameObject &entity, const Mat4 &newWorldMat);
};

} // namespace Elysium

#endif // TRANSFORMGIZMO_HPP
