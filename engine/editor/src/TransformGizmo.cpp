#include "elypch.hpp"
#include "TransformGizmo.hpp"
#include "Scene.hpp"
#include <cstdio>

namespace Elysium {

namespace {

constexpr float kHandleLength = 75.0f;
constexpr float kArrowTipLength = 14.0f;
constexpr float kArrowHalfWidth = 6.0f;
constexpr float kCenterQuadSize = 18.0f;
constexpr float kCenterBoxHalf = 7.0f;
constexpr float kRotateRadius = 70.0f;
constexpr float kHitThreshold = 7.0f;
constexpr float kPi = 3.14159265358979323846f;

float DistancePointToSegment(const ImVec2 &p, const ImVec2 &a, const ImVec2 &b) {
  float l2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
  if (l2 < 0.0001f)
    return std::hypot(p.x - a.x, p.y - a.y);
  float t = std::clamp(((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2, 0.0f, 1.0f);
  ImVec2 proj(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y));
  return std::hypot(p.x - proj.x, p.y - proj.y);
}

} // namespace

void TransformGizmo::Reset() {
  m_hoveredHandle = GizmoHandle::None;
  m_activeHandle = GizmoHandle::None;
  m_activeEntityUUID = 0;
  m_currentAngleDeltaRad = 0.0f;
}

ImVec2 TransformGizmo::WorldToScreen(const Vec3 &worldPos, const EditorContext &context,
                                     const ImVec2 &imageMin, const ImVec2 &imageSize) const {
  float effectivePtm = context.pixelsPerMeter * context.camera.zoom;
  if (effectivePtm <= 0.0001f)
    effectivePtm = 1.0f;
  float vx = (worldPos.x - context.camera.center.x) * effectivePtm;
  float vy = (worldPos.y - context.camera.center.y) * effectivePtm;
  return ImVec2(imageMin.x + imageSize.x * 0.5f + vx,
                imageMin.y + imageSize.y * 0.5f + vy);
}

void TransformGizmo::Render(EditorContext &context, Scene &activeScene,
                            const ImVec2 &imageMin, const ImVec2 &imageSize,
                            ImDrawList *drawList) {
  (void)activeScene;

  // Gizmos only operate in Edit Mode when an entity is selected and an operation is active
  if (context.playState != ScenePlayState::Edit || !context.selectedEntity ||
      context.gizmoOperation == GizmoOperation::None) {
    Reset();
    return;
  }

  GameObject &entity = *context.selectedEntity;
  uint64_t currentUUID = static_cast<uint64_t>(entity.GetUUID());
  if (m_activeEntityUUID != currentUUID && m_activeHandle == GizmoHandle::None) {
    m_activeEntityUUID = currentUUID;
  }

  ImVec2 mousePos = ImGui::GetMousePos();
  bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
  bool isMouseClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

  // If user released the mouse button, terminate any active drag
  if (!isMouseDown && m_activeHandle != GizmoHandle::None) {
    m_activeHandle = GizmoHandle::None;
  }

  // Determine entity screen origin
  Vec3 worldPos = entity.GetWorldPosition();
  ImVec2 originScreen = WorldToScreen(worldPos, context, imageMin, imageSize);

  // Compute reference axes in screen space (World vs. Local)
  Vec2 axisX(1.0f, 0.0f);
  Vec2 axisY(0.0f, 1.0f);

  if (context.gizmoMode == GizmoMode::Local || context.gizmoOperation == GizmoOperation::Scale) {
    Quat rot = entity.GetWorldRotation();
    float angleRad = 2.0f * std::atan2(rot.z, rot.w);
    axisX = Vec2(std::cos(angleRad), std::sin(angleRad));
    axisY = Vec2(-std::sin(angleRad), std::cos(angleRad));
  }

  // Reset hovered handle if not currently dragging
  if (m_activeHandle == GizmoHandle::None) {
    m_hoveredHandle = GizmoHandle::None;
  }

  // Clip gizmo drawing to viewport bounds
  drawList->PushClipRect(imageMin, ImVec2(imageMin.x + imageSize.x, imageMin.y + imageSize.y), true);

  switch (context.gizmoOperation) {
  case GizmoOperation::Translate:
    RenderTranslateGizmo(context, entity, originScreen, axisX, axisY, drawList,
                         mousePos, isMouseDown, isMouseClicked);
    break;
  case GizmoOperation::Rotate:
    RenderRotateGizmo(context, entity, originScreen, drawList, mousePos,
                      isMouseDown, isMouseClicked);
    break;
  case GizmoOperation::Scale:
    RenderScaleGizmo(context, entity, originScreen, axisX, axisY, drawList,
                     mousePos, isMouseDown, isMouseClicked);
    break;
  default:
    break;
  }

  drawList->PopClipRect();
}

void TransformGizmo::RenderTranslateGizmo(EditorContext &context, GameObject &entity,
                                         const ImVec2 &originScreen,
                                         const Vec2 &axisX, const Vec2 &axisY,
                                         ImDrawList *drawList, const ImVec2 &mousePos,
                                         bool isMouseDown, bool isMouseClicked) {
  ImVec2 endX(originScreen.x + axisX.x * kHandleLength,
              originScreen.y + axisX.y * kHandleLength);
  ImVec2 endY(originScreen.x + axisY.x * kHandleLength,
              originScreen.y + axisY.y * kHandleLength);

  // Quad vertices for planar translation
  ImVec2 q0 = originScreen;
  ImVec2 q1(originScreen.x + axisX.x * kCenterQuadSize,
            originScreen.y + axisX.y * kCenterQuadSize);
  ImVec2 q2(originScreen.x + (axisX.x + axisY.x) * kCenterQuadSize,
            originScreen.y + (axisX.y + axisY.y) * kCenterQuadSize);
  ImVec2 q3(originScreen.x + axisY.x * kCenterQuadSize,
            originScreen.y + axisY.y * kCenterQuadSize);

  // Hit testing when not already dragging
  if (m_activeHandle == GizmoHandle::None) {
    // 1. Center planar quad
    float distQuad = std::hypot(mousePos.x - (q0.x + q2.x) * 0.5f,
                                mousePos.y - (q0.y + q2.y) * 0.5f);
    if (distQuad <= kCenterQuadSize * 0.75f) {
      m_hoveredHandle = GizmoHandle::TranslateXY;
    } else {
      // 2. X and Y axis handles
      float distX = DistancePointToSegment(mousePos, originScreen, endX);
      float distY = DistancePointToSegment(mousePos, originScreen, endY);

      if (distX <= kHitThreshold && distX <= distY) {
        m_hoveredHandle = GizmoHandle::TranslateX;
      } else if (distY <= kHitThreshold) {
        m_hoveredHandle = GizmoHandle::TranslateY;
      } else {
        m_hoveredHandle = GizmoHandle::None;
      }
    }

    if (isMouseClicked && m_hoveredHandle != GizmoHandle::None) {
      m_activeHandle = m_hoveredHandle;
      m_dragStartMouse = mousePos;
      m_initialWorldPos = entity.GetWorldPosition();
      m_initialWorldRot = entity.GetWorldRotation();
      m_initialWorldScale = entity.GetWorldScale();
      m_initialLocalPos = entity.GetLocalPosition();
      m_initialLocalRot = entity.GetLocalRotation();
      m_initialLocalScale = entity.GetLocalScale();
    }
  }

  // Active dragging computation
  if (m_activeHandle != GizmoHandle::None && isMouseDown) {
    float effectivePtm = context.pixelsPerMeter * context.camera.zoom;
    if (effectivePtm <= 0.0001f)
      effectivePtm = 1.0f;

    ImVec2 mouseDelta(mousePos.x - m_dragStartMouse.x, mousePos.y - m_dragStartMouse.y);
    bool snapActive = context.snapEnabled || ImGui::GetIO().KeyCtrl;
    float snapStep = context.snapTranslation;
    Vec3 newPos = m_initialWorldPos;

    if (m_activeHandle == GizmoHandle::TranslateXY) {
      Vec3 worldDelta(mouseDelta.x / effectivePtm, mouseDelta.y / effectivePtm, 0.0f);
      newPos = m_initialWorldPos + worldDelta;
      if (snapActive && snapStep > 0.001f) {
        newPos.x = std::round(newPos.x / snapStep) * snapStep;
        newPos.y = std::round(newPos.y / snapStep) * snapStep;
      }
    } else if (m_activeHandle == GizmoHandle::TranslateX) {
      float proj = mouseDelta.x * axisX.x + mouseDelta.y * axisX.y;
      float worldDist = proj / effectivePtm;
      if (snapActive && snapStep > 0.001f) {
        worldDist = std::round(worldDist / snapStep) * snapStep;
      }
      newPos = m_initialWorldPos + Vec3(axisX.x * worldDist, axisX.y * worldDist, 0.0f);
    } else if (m_activeHandle == GizmoHandle::TranslateY) {
      float proj = mouseDelta.x * axisY.x + mouseDelta.y * axisY.y;
      float worldDist = proj / effectivePtm;
      if (snapActive && snapStep > 0.001f) {
        worldDist = std::round(worldDist / snapStep) * snapStep;
      }
      newPos = m_initialWorldPos + Vec3(axisY.x * worldDist, axisY.y * worldDist, 0.0f);
    }

    Mat4 newWorldMat = Mat4::TRS(newPos, m_initialWorldRot, m_initialWorldScale);
    ApplyWorldTransform(entity, newWorldMat);
  }

  // Drawing
  ImU32 colX = (m_activeHandle == GizmoHandle::TranslateX || m_hoveredHandle == GizmoHandle::TranslateX)
                   ? IM_COL32(255, 255, 0, 255)
                   : IM_COL32(235, 60, 60, 255);
  ImU32 colY = (m_activeHandle == GizmoHandle::TranslateY || m_hoveredHandle == GizmoHandle::TranslateY)
                   ? IM_COL32(255, 255, 0, 255)
                   : IM_COL32(60, 220, 80, 255);
  ImU32 colQuadFill = (m_activeHandle == GizmoHandle::TranslateXY || m_hoveredHandle == GizmoHandle::TranslateXY)
                          ? IM_COL32(255, 255, 0, 140)
                          : IM_COL32(240, 240, 240, 80);
  ImU32 colQuadBorder = (m_activeHandle == GizmoHandle::TranslateXY || m_hoveredHandle == GizmoHandle::TranslateXY)
                            ? IM_COL32(255, 255, 0, 255)
                            : IM_COL32(240, 240, 240, 200);

  // Planar Quad
  drawList->AddQuadFilled(q0, q1, q2, q3, colQuadFill);
  drawList->AddQuad(q0, q1, q2, q3, colQuadBorder, 1.5f);

  // X Axis Line & Arrow
  drawList->AddLine(originScreen, endX, colX, 3.0f);
  ImVec2 xBase(endX.x - axisX.x * kArrowTipLength, endX.y - axisX.y * kArrowTipLength);
  ImVec2 xLeft(xBase.x - axisX.y * kArrowHalfWidth, xBase.y + axisX.x * kArrowHalfWidth);
  ImVec2 xRight(xBase.x + axisX.y * kArrowHalfWidth, xBase.y - axisX.x * kArrowHalfWidth);
  drawList->AddTriangleFilled(endX, xLeft, xRight, colX);

  // Y Axis Line & Arrow
  drawList->AddLine(originScreen, endY, colY, 3.0f);
  ImVec2 yBase(endY.x - axisY.x * kArrowTipLength, endY.y - axisY.y * kArrowTipLength);
  ImVec2 yLeft(yBase.x - axisY.y * kArrowHalfWidth, yBase.y + axisY.x * kArrowHalfWidth);
  ImVec2 yRight(yBase.x + axisY.y * kArrowHalfWidth, yBase.y - axisY.x * kArrowHalfWidth);
  drawList->AddTriangleFilled(endY, yLeft, yRight, colY);

  // Center pivot dot
  drawList->AddCircleFilled(originScreen, 3.5f, IM_COL32(255, 255, 255, 255));
}

void TransformGizmo::RenderRotateGizmo(EditorContext &context, GameObject &entity,
                                      const ImVec2 &originScreen,
                                      ImDrawList *drawList, const ImVec2 &mousePos,
                                      bool isMouseDown, bool isMouseClicked) {
  float distToCenter = std::hypot(mousePos.x - originScreen.x, mousePos.y - originScreen.y);

  if (m_activeHandle == GizmoHandle::None) {
    if (std::abs(distToCenter - kRotateRadius) <= kHitThreshold + 2.0f) {
      m_hoveredHandle = GizmoHandle::RotateRing;
    } else {
      m_hoveredHandle = GizmoHandle::None;
    }

    if (isMouseClicked && m_hoveredHandle == GizmoHandle::RotateRing) {
      m_activeHandle = GizmoHandle::RotateRing;
      m_dragStartMouse = mousePos;
      m_initialWorldPos = entity.GetWorldPosition();
      m_initialWorldRot = entity.GetWorldRotation();
      m_initialWorldScale = entity.GetWorldScale();
      m_initialAngleRad = std::atan2(mousePos.y - originScreen.y, mousePos.x - originScreen.x);
      m_currentAngleDeltaRad = 0.0f;
    }
  }

  // Active rotation dragging
  if (m_activeHandle == GizmoHandle::RotateRing && isMouseDown) {
    float curAngle = std::atan2(mousePos.y - originScreen.y, mousePos.x - originScreen.x);
    float deltaAngle = curAngle - m_initialAngleRad;

    bool snapActive = context.snapEnabled || ImGui::GetIO().KeyCtrl;
    float snapDeg = context.snapRotation;
    if (snapActive && snapDeg > 0.001f) {
      float snapRad = snapDeg * (kPi / 180.0f);
      deltaAngle = std::round(deltaAngle / snapRad) * snapRad;
    }
    m_currentAngleDeltaRad = deltaAngle;

    // Apply rotation around Z-axis
    Quat deltaRot(Vec3(0.0f, 0.0f, 1.0f), deltaAngle);
    Quat newRot = deltaRot * m_initialWorldRot;
    Mat4 newWorldMat = Mat4::TRS(m_initialWorldPos, newRot, m_initialWorldScale);
    ApplyWorldTransform(entity, newWorldMat);
  }

  ImU32 colRing = (m_activeHandle == GizmoHandle::RotateRing || m_hoveredHandle == GizmoHandle::RotateRing)
                      ? IM_COL32(255, 255, 0, 255)
                      : IM_COL32(50, 180, 250, 255);

  // Rotation ring
  drawList->AddCircle(originScreen, kRotateRadius, colRing, 64, 2.5f);

  // Live angle delta visual arc and readout during drag
  if (m_activeHandle == GizmoHandle::RotateRing) {
    ImVec2 startDir(originScreen.x + std::cos(m_initialAngleRad) * kRotateRadius,
                    originScreen.y + std::sin(m_initialAngleRad) * kRotateRadius);
    ImVec2 curDir(originScreen.x + std::cos(m_initialAngleRad + m_currentAngleDeltaRad) * kRotateRadius,
                  originScreen.y + std::sin(m_initialAngleRad + m_currentAngleDeltaRad) * kRotateRadius);

    drawList->AddLine(originScreen, startDir, IM_COL32(255, 255, 255, 180), 1.5f);
    drawList->AddLine(originScreen, curDir, IM_COL32(255, 255, 0, 255), 2.0f);

    // Draw filled arc sector
    float aMin = std::min(m_initialAngleRad, m_initialAngleRad + m_currentAngleDeltaRad);
    float aMax = std::max(m_initialAngleRad, m_initialAngleRad + m_currentAngleDeltaRad);
    drawList->PathLineTo(originScreen);
    drawList->PathArcTo(originScreen, kRotateRadius, aMin, aMax, 32);
    drawList->PathFillConvex(IM_COL32(255, 255, 0, 60));

    // Live angle tooltip / text readout
    char angleBuf[32];
    float degrees = m_currentAngleDeltaRad * (180.0f / kPi);
    std::snprintf(angleBuf, sizeof(angleBuf), "%.1f deg", static_cast<double>(degrees));
    ImVec2 textPos(curDir.x + 12.0f, curDir.y - 8.0f);
    drawList->AddRectFilled(ImVec2(textPos.x - 4.0f, textPos.y - 2.0f),
                            ImVec2(textPos.x + 60.0f, textPos.y + 16.0f),
                            IM_COL32(20, 20, 20, 200), 3.0f);
    drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), angleBuf);
  }

  // Center pivot dot
  drawList->AddCircleFilled(originScreen, 3.5f, IM_COL32(255, 255, 255, 255));
}

void TransformGizmo::RenderScaleGizmo(EditorContext &context, GameObject &entity,
                                     const ImVec2 &originScreen,
                                     const Vec2 &axisX, const Vec2 &axisY,
                                     ImDrawList *drawList, const ImVec2 &mousePos,
                                     bool isMouseDown, bool isMouseClicked) {
  ImVec2 endX(originScreen.x + axisX.x * kHandleLength,
              originScreen.y + axisX.y * kHandleLength);
  ImVec2 endY(originScreen.x + axisY.x * kHandleLength,
              originScreen.y + axisY.y * kHandleLength);

  // Center uniform scale box bounds
  ImVec2 b0(originScreen.x - kCenterBoxHalf, originScreen.y - kCenterBoxHalf);
  ImVec2 b1(originScreen.x + kCenterBoxHalf, originScreen.y + kCenterBoxHalf);

  if (m_activeHandle == GizmoHandle::None) {
    if (mousePos.x >= b0.x && mousePos.x <= b1.x &&
        mousePos.y >= b0.y && mousePos.y <= b1.y) {
      m_hoveredHandle = GizmoHandle::ScaleXY;
    } else {
      float distX = DistancePointToSegment(mousePos, originScreen, endX);
      float distY = DistancePointToSegment(mousePos, originScreen, endY);

      if (distX <= kHitThreshold && distX <= distY) {
        m_hoveredHandle = GizmoHandle::ScaleX;
      } else if (distY <= kHitThreshold) {
        m_hoveredHandle = GizmoHandle::ScaleY;
      } else {
        m_hoveredHandle = GizmoHandle::None;
      }
    }

    if (isMouseClicked && m_hoveredHandle != GizmoHandle::None) {
      m_activeHandle = m_hoveredHandle;
      m_dragStartMouse = mousePos;
      m_initialWorldPos = entity.GetWorldPosition();
      m_initialWorldRot = entity.GetWorldRotation();
      m_initialWorldScale = entity.GetWorldScale();
      m_initialLocalPos = entity.GetLocalPosition();
      m_initialLocalRot = entity.GetLocalRotation();
      m_initialLocalScale = entity.GetLocalScale();
    }
  }

  // Active scale dragging
  if (m_activeHandle != GizmoHandle::None && isMouseDown) {
    ImVec2 mouseDelta(mousePos.x - m_dragStartMouse.x, mousePos.y - m_dragStartMouse.y);
    bool snapActive = context.snapEnabled || ImGui::GetIO().KeyCtrl;
    float snapStep = context.snapScale;
    Vec3 newScale = m_initialLocalScale;

    if (m_activeHandle == GizmoHandle::ScaleXY) {
      float delta = (mouseDelta.x - mouseDelta.y) / 100.0f;
      if (snapActive && snapStep > 0.001f) {
        delta = std::round(delta / snapStep) * snapStep;
      }
      float factor = std::max(0.01f, 1.0f + delta);
      newScale = m_initialLocalScale * factor;
    } else if (m_activeHandle == GizmoHandle::ScaleX) {
      float proj = mouseDelta.x * axisX.x + mouseDelta.y * axisX.y;
      float delta = proj / kHandleLength;
      if (snapActive && snapStep > 0.001f) {
        delta = std::round(delta / snapStep) * snapStep;
      }
      float factor = std::max(0.01f, 1.0f + delta);
      newScale.x = m_initialLocalScale.x * factor;
    } else if (m_activeHandle == GizmoHandle::ScaleY) {
      float proj = mouseDelta.x * axisY.x + mouseDelta.y * axisY.y;
      float delta = proj / kHandleLength;
      if (snapActive && snapStep > 0.001f) {
        delta = std::round(delta / snapStep) * snapStep;
      }
      float factor = std::max(0.01f, 1.0f + delta);
      newScale.y = m_initialLocalScale.y * factor;
    }

    entity.scale = newScale;
    entity.SyncTransform();
  }

  // Drawing
  ImU32 colX = (m_activeHandle == GizmoHandle::ScaleX || m_hoveredHandle == GizmoHandle::ScaleX)
                   ? IM_COL32(255, 255, 0, 255)
                   : IM_COL32(235, 60, 60, 255);
  ImU32 colY = (m_activeHandle == GizmoHandle::ScaleY || m_hoveredHandle == GizmoHandle::ScaleY)
                   ? IM_COL32(255, 255, 0, 255)
                   : IM_COL32(60, 220, 80, 255);
  ImU32 colBox = (m_activeHandle == GizmoHandle::ScaleXY || m_hoveredHandle == GizmoHandle::ScaleXY)
                     ? IM_COL32(255, 255, 0, 255)
                     : IM_COL32(240, 240, 240, 255);

  // X Axis Line & Box Cap
  drawList->AddLine(originScreen, endX, colX, 3.0f);
  drawList->AddRectFilled(ImVec2(endX.x - 5.0f, endX.y - 5.0f),
                          ImVec2(endX.x + 5.0f, endX.y + 5.0f), colX);

  // Y Axis Line & Box Cap
  drawList->AddLine(originScreen, endY, colY, 3.0f);
  drawList->AddRectFilled(ImVec2(endY.x - 5.0f, endY.y - 5.0f),
                          ImVec2(endY.y + 5.0f, endY.y + 5.0f), colY);

  // Center uniform scale box
  drawList->AddRectFilled(b0, b1, colBox);
  drawList->AddRect(b0, b1, IM_COL32(0, 0, 0, 180), 0.0f, 0, 1.0f);
}

void TransformGizmo::ApplyWorldTransform(GameObject &entity, const Mat4 &newWorldMat) {
  if (entity.GetTransform().ParentHandle != 0 && entity.GetScene()) {
    GameObject *parent = entity.GetScene()->GetEntityByUUID(entity.GetTransform().ParentHandle);
    if (parent) {
      Mat4 parentWorldInv = parent->GetWorldTransform().Inverse();
      Mat4 localMat = parentWorldInv * newWorldMat;
      entity.position = localMat.GetTranslation();
      entity.rotation = localMat.GetRotation();
      entity.scale = localMat.GetScale();
    } else {
      entity.position = newWorldMat.GetTranslation();
      entity.rotation = newWorldMat.GetRotation();
      entity.scale = newWorldMat.GetScale();
    }
  } else {
    entity.position = newWorldMat.GetTranslation();
    entity.rotation = newWorldMat.GetRotation();
    entity.scale = newWorldMat.GetScale();
  }

  entity.SyncTransform();
}

} // namespace Elysium
