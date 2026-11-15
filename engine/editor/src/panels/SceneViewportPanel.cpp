#include "elypch.hpp"
#include "panels/SceneViewportPanel.hpp"
#include <imgui-SFML.h>

namespace Elysium {

SceneViewportPanel::SceneViewportPanel() : EditorPanel("Scene Viewport") {}

void SceneViewportPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                       SimpleRenderer &renderer) {
  if (m_focusRequested)
    ImGui::SetNextWindowFocus();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  const bool visible = ImGui::Begin("Scene Viewport", &isOpen,
                                    ImGuiWindowFlags_NoScrollbar |
                                        ImGuiWindowFlags_NoScrollWithMouse);
  // Known even while another tab hides this one
  context.viewportDockId = ImGui::GetWindowDockID();
  // Right after the layout is rebuilt, the dock node can still pick another
  // tab to show, so keep asking until this tab is the one in front
  if (m_focusRequested && !ImGui::IsWindowAppearing() &&
      (visible || !ImGui::IsWindowDocked()))
    m_focusRequested = false;
  if (visible) {
    ImVec2 viewportAvail = ImGui::GetContentRegionAvail();
    context.viewportPos = sf::Vector2f(ImGui::GetCursorScreenPos().x,
                                       ImGui::GetCursorScreenPos().y);
    context.viewportSize = sf::Vector2f(viewportAvail.x, viewportAvail.y);
    context.isViewportHovered = ImGui::IsWindowHovered();
    context.isViewportFocused = ImGui::IsWindowFocused();

    // Ensure valid minimum dimensions
    unsigned int width =
        static_cast<unsigned int>(std::max(1.0f, viewportAvail.x));
    unsigned int height =
        static_cast<unsigned int>(std::max(1.0f, viewportAvail.y));

    // Dynamic resize of the off-screen RenderTexture. After a failed resize
    // the old target can't be trusted, so skip drawing and retry next frame
    bool targetReady = viewportAvail.x >= 1.0f && viewportAvail.y >= 1.0f;
    if (targetReady &&
        (m_currentTextureSize.x != width || m_currentTextureSize.y != height)) {
      if (m_viewportTexture.resize(sf::Vector2u(width, height))) {
        m_currentTextureSize = sf::Vector2u(width, height);
        scene.OnViewportResize(width, height);
        if (context.runtimeScene) {
          context.runtimeScene->OnViewportResize(width, height);
        }
      } else {
        m_currentTextureSize = {0, 0};
        targetReady = false;
      }
    }

    // Determine active scene and play state
    bool isPlaying = (context.playState != ScenePlayState::Edit);
    Scene &activeScene = (isPlaying && context.runtimeScene) ? *context.runtimeScene : scene;
    GameObject *primaryCamEntity = isPlaying ? activeScene.GetPrimaryCameraEntity() : nullptr;
    CameraComponent *primaryCamComp = primaryCamEntity ? primaryCamEntity->GetComponent<CameraComponent>() : nullptr;

    // Configure camera view
    float ptm = context.pixelsPerMeter;
    float zoom = context.camera.zoom;
    sf::View view;

    if (isPlaying && primaryCamComp && primaryCamEntity) {
      // Play mode: render through the primary camera entity
      view = primaryCamComp->Camera.CalculateView(
          primaryCamEntity->GetWorldPosition(), primaryCamEntity->GetWorldRotation(), ptm);
      renderer.clearColor = primaryCamComp->ClearColor;
      renderer.drawCameraFrustum = false;
    } else {
      // Edit mode (or play mode fallback): render through EditorCamera
      view.setSize({static_cast<float>(width), static_cast<float>(height)});
      view.setCenter(
          {context.camera.center.x * ptm, context.camera.center.y * ptm});
      view.zoom(1.0f / zoom);
      renderer.clearColor = sf::Color(56, 56, 56, 255);
      renderer.drawCameraFrustum = true;
    }

    // Render active scene to off-screen RenderTexture
    renderer.drawDebugGrid = context.drawDebugGrid;
    renderer.drawPhysicsColliders = context.drawPhysicsColliders;
    renderer.drawAABBs = context.drawAABBs;
    renderer.drawBoundaryBox = context.drawBoundaryBox;
    renderer.pixelsPerMeter = context.pixelsPerMeter;
    renderer.selectedEntity = context.selectedEntity;
    if (targetReady) {
      renderer.Render(m_viewportTexture, activeScene, view);
      m_viewportTexture.display();

      // Display texture in ImGui
      ImGui::Image(m_viewportTexture);
    }

    ImVec2 imageMin = ImGui::GetItemRectMin();
    ImVec2 imageSize = ImGui::GetItemRectSize();
    bool isImageHovered = ImGui::IsItemHovered();

    // Render 2D reference grid & origin axes (Edit Mode only)
    if (!isPlaying) {
      RenderGridAndAxes(context, imageMin, imageSize);
    }

    // Render interactive transform gizmo (Edit Mode only)
    if (!isPlaying && context.selectedEntity && context.gizmoOperation != GizmoOperation::None) {
      m_gizmo.Render(context, activeScene, imageMin, imageSize, ImGui::GetWindowDrawList());
    } else {
      m_gizmo.Reset();
    }

    // Viewport HUD Overlay (Camera info & quick controls)
    ImVec2 overlayPos =
        ImVec2(context.viewportPos.x + 10.0f, context.viewportPos.y + 10.0f);
    ImGui::SetNextWindowPos(overlayPos);
    ImGui::SetNextWindowBgAlpha(0.6f);
    if (ImGui::BeginChild("ViewportHUD", ImVec2(240.0f, 65.0f), true,
                          ImGuiWindowFlags_NoDecoration |
                              ImGuiWindowFlags_NoDocking |
                              ImGuiWindowFlags_NoSavedSettings)) {
      if (isPlaying && primaryCamEntity) {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[Play Mode - Camera]");
        ImGui::Text("Cam: %s", primaryCamEntity->name.c_str());
      } else {
        ImGui::Text("Cam: (%.2f, %.2f) m",
                    static_cast<double>(context.camera.center.x),
                    static_cast<double>(context.camera.center.y));
        ImGui::Text("Zoom: %.0f%%", static_cast<double>(context.camera.zoom * 100.0f));
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset View")) {
          context.camera.Reset();
        }
      }
    }
    ImGui::EndChild();

    // Viewport Mouse Picking / Entity Selection
    // Guard rails: Only trigger on Left-Click when hovered over the viewport image,
    // not dragging, not over the HUD overlay, not interacting with other ImGui items,
    // and not hovering or using any transform gizmo handle.
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 hudMin = overlayPos;
    ImVec2 hudMax = ImVec2(overlayPos.x + 240.0f, overlayPos.y + 65.0f);
    bool overHud = (mousePos.x >= hudMin.x && mousePos.x <= hudMax.x &&
                    mousePos.y >= hudMin.y && mousePos.y <= hudMax.y);

    bool leftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool isDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
                      ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
                      ImGui::IsMouseDragging(ImGuiMouseButton_Middle);

    if (isImageHovered && leftClicked && !isDragging && !overHud && !ImGui::IsAnyItemActive() &&
        !m_gizmo.IsOver() && !m_gizmo.IsUsing()) {
      sf::Vector2f mouseInViewport(mousePos.x - imageMin.x, mousePos.y - imageMin.y);
      Vec2 worldPos(0.0f, 0.0f);

      if (isPlaying && primaryCamComp && primaryCamEntity) {
        // Unproject through active primary camera
        float orthoH = primaryCamComp->Camera.GetOrthographicSize();
        float orthoW = orthoH * primaryCamComp->Camera.GetAspectRatio();
        float normX = (mouseInViewport.x / imageSize.x) - 0.5f;
        float normY = (mouseInViewport.y / imageSize.y) - 0.5f;
        float localX = normX * orthoW;
        float localY = normY * orthoH;
        Quat camRot = primaryCamEntity->GetWorldRotation();
        float angleRad = 2.0f * std::atan2(camRot.z, camRot.w);
        float rx = localX * std::cos(angleRad) - localY * std::sin(angleRad);
        float ry = localX * std::sin(angleRad) + localY * std::cos(angleRad);
        Vec3 camWorldPos = primaryCamEntity->GetWorldPosition();
        worldPos = Vec2(camWorldPos.x + rx, camWorldPos.y + ry);
      } else {
        sf::Vector2f unprojected = context.camera.ViewportToWorld(
            mouseInViewport, sf::Vector2f(imageSize.x, imageSize.y), ptm);
        worldPos = Vec2(unprojected.x, unprojected.y);
      }

      auto picked = renderer.PickEntity(activeScene, worldPos);
      context.selectedEntity = picked;
    }

    // Independent Viewport Camera Controls (Edit Mode only, when not using gizmo)
    if (!isPlaying && context.isViewportHovered && !overHud && !m_gizmo.IsUsing()) {
      // Zooming via mouse wheel
      float wheel = ImGui::GetIO().MouseWheel;
      if (wheel != 0.0f) {
        context.camera.zoom += wheel * 0.15f * context.camera.zoom;
        context.camera.zoom = std::clamp(context.camera.zoom, 0.05f, 20.0f);
      }

      // Panning via Right Mouse Button or Middle Mouse Button
      if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
        context.camera.center.x -= dragDelta.x / (ptm * zoom);
        context.camera.center.y -= dragDelta.y / (ptm * zoom);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
      } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
        context.camera.center.x -= dragDelta.x / (ptm * zoom);
        context.camera.center.y -= dragDelta.y / (ptm * zoom);
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
      }
    }
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

void SceneViewportPanel::RenderGridAndAxes(EditorContext &context, const ImVec2 &imageMin,
                                           const ImVec2 &imageSize) {
  if (context.playState != ScenePlayState::Edit)
    return;

  if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
    return;

  ImDrawList *drawList = ImGui::GetWindowDrawList();
  ImVec2 imageMax = ImVec2(imageMin.x + imageSize.x, imageMin.y + imageSize.y);
  drawList->PushClipRect(imageMin, imageMax, true);

  float effectivePtm = context.pixelsPerMeter * context.camera.zoom;
  if (effectivePtm <= 0.0001f)
    effectivePtm = 1.0f;

  float halfWidth = imageSize.x * 0.5f;
  float halfHeight = imageSize.y * 0.5f;

  float worldMinX = context.camera.center.x - halfWidth / effectivePtm;
  float worldMaxX = context.camera.center.x + halfWidth / effectivePtm;
  float worldMinY = context.camera.center.y - halfHeight / effectivePtm;
  float worldMaxY = context.camera.center.y + halfHeight / effectivePtm;

  // 1. 2D Reference Snap Grid
  if (context.ShowGrid) {
    float baseSpacing = (context.GridSnapSize > 0.001f) ? context.GridSnapSize : 1.0f;
    float cellSpacing = baseSpacing;

    // Zoom-adaptive: if screen pixel spacing between lines drops below 8.0f pixels,
    // step up the subdivision multiple (2x, 5x, 10x, etc.) so lines never moiré or turn solid.
    while (cellSpacing * effectivePtm < 8.0f) {
      float px = cellSpacing * effectivePtm;
      if (px * 2.0f >= 8.0f) {
        cellSpacing *= 2.0f;
      } else if (px * 5.0f >= 8.0f) {
        cellSpacing *= 5.0f;
      } else {
        cellSpacing *= 10.0f;
      }
    }

    // Smoothly fade minor line alpha when pixel spacing approaches 8.0f
    float pixelSpacing = cellSpacing * effectivePtm;
    float alphaFade = std::clamp((pixelSpacing - 8.0f) / 16.0f, 0.0f, 1.0f);
    int minorAlpha = static_cast<int>(18.0f * alphaFade);
    int majorAlpha = 38;

    ImU32 colMajor = IM_COL32(255, 255, 255, majorAlpha);
    ImU32 colMinor = IM_COL32(255, 255, 255, minorAlpha);

    // Vertical grid lines (constant world X)
    int64_t startIdxX = static_cast<int64_t>(std::floor(worldMinX / cellSpacing));
    int64_t endIdxX = static_cast<int64_t>(std::ceil(worldMaxX / cellSpacing));
    for (int64_t idx = startIdxX; idx <= endIdxX; ++idx) {
      bool isMajor = (idx % 5 == 0);
      if (!isMajor && minorAlpha <= 0)
        continue;
      ImU32 col = isMajor ? colMajor : colMinor;
      float worldX = static_cast<float>(idx) * cellSpacing;
      float screenX = imageMin.x + halfWidth + (worldX - context.camera.center.x) * effectivePtm;
      drawList->AddLine(ImVec2(screenX, imageMin.y), ImVec2(screenX, imageMax.y), col, 1.0f);
    }

    // Horizontal grid lines (constant world Y)
    int64_t startIdxY = static_cast<int64_t>(std::floor(worldMinY / cellSpacing));
    int64_t endIdxY = static_cast<int64_t>(std::ceil(worldMaxY / cellSpacing));
    for (int64_t idx = startIdxY; idx <= endIdxY; ++idx) {
      bool isMajor = (idx % 5 == 0);
      if (!isMajor && minorAlpha <= 0)
        continue;
      ImU32 col = isMajor ? colMajor : colMinor;
      float worldY = static_cast<float>(idx) * cellSpacing;
      float screenY = imageMin.y + halfHeight + (worldY - context.camera.center.y) * effectivePtm;
      drawList->AddLine(ImVec2(imageMin.x, screenY), ImVec2(imageMax.x, screenY), col, 1.0f);
    }
  }

  // 2. Infinite X & Y World Origin Axes
  if (context.ShowAxes) {
    // World X-Axis: horizontal line at world y = 0
    float screenOriginY = imageMin.y + halfHeight + (0.0f - context.camera.center.y) * effectivePtm;
    drawList->AddLine(ImVec2(imageMin.x, screenOriginY), ImVec2(imageMax.x, screenOriginY),
                      IM_COL32(205, 70, 70, 185), 1.5f);

    // World Y-Axis: vertical line at world x = 0
    float screenOriginX = imageMin.x + halfWidth + (0.0f - context.camera.center.x) * effectivePtm;
    drawList->AddLine(ImVec2(screenOriginX, imageMin.y), ImVec2(screenOriginX, imageMax.y),
                      IM_COL32(75, 200, 90, 185), 1.5f);
  }

  drawList->PopClipRect();
}

} // namespace Elysium
