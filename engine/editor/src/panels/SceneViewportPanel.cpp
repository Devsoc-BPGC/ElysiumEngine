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
      renderer.clearColor = sf::Color(119, 221, 119);
      renderer.drawCameraFrustum = true;
    }

    // Render active scene to off-screen RenderTexture
    renderer.drawDebugGrid = context.drawDebugGrid;
    renderer.drawPhysicsColliders = context.drawPhysicsColliders;
    renderer.drawAABBs = context.drawAABBs;
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
        ImGui::Text("Cam: (%.2f, %.2f) m", context.camera.center.x,
                    context.camera.center.y);
        ImGui::Text("Zoom: %.0f%%", context.camera.zoom * 100.0f);
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

} // namespace Elysium
