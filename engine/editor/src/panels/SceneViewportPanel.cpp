#include "panels/SceneViewportPanel.hpp"
#include <algorithm>
#include <imgui-SFML.h>
#include <imgui.h>

namespace Elysium {

SceneViewportPanel::SceneViewportPanel() : EditorPanel("Viewport") {}

void SceneViewportPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                      SimpleRenderer &renderer) {
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::Begin("Viewport", &isOpen,
                   ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse)) {
    ImVec2 viewportAvail = ImGui::GetContentRegionAvail();
    context.viewportPos = sf::Vector2f(ImGui::GetCursorScreenPos().x,
                                       ImGui::GetCursorScreenPos().y);
    context.viewportSize = sf::Vector2f(viewportAvail.x, viewportAvail.y);
    context.isViewportHovered = ImGui::IsWindowHovered();
    context.isViewportFocused = ImGui::IsWindowFocused();

    // Ensure valid minimum dimensions
    unsigned int width = static_cast<unsigned int>(std::max(1.0f, viewportAvail.x));
    unsigned int height = static_cast<unsigned int>(std::max(1.0f, viewportAvail.y));

    // Dynamic resize of the off-screen RenderTexture
    if (m_currentTextureSize.x != width || m_currentTextureSize.y != height) {
      if (m_viewportTexture.resize(sf::Vector2u(width, height))) {
        m_currentTextureSize = sf::Vector2u(width, height);
      }
    }

    // Configure camera view
    float ptm = context.pixelsPerMeter;
    float zoom = context.camera.zoom;
    sf::View view;
    view.setSize({static_cast<float>(width), static_cast<float>(height)});
    view.setCenter({context.camera.center.x * ptm, context.camera.center.y * ptm});
    view.zoom(1.0f / zoom);

    // Render active scene to off-screen RenderTexture
    renderer.drawDebugGrid = context.drawDebugGrid;
    renderer.drawPhysicsColliders = context.drawPhysicsColliders;
    renderer.drawAABBs = context.drawAABBs;
    renderer.pixelsPerMeter = context.pixelsPerMeter;
    renderer.Render(m_viewportTexture, scene, view);
    m_viewportTexture.display();

    // Display texture in ImGui
    ImGui::Image(m_viewportTexture);

    // Independent Viewport Camera Controls
    if (context.isViewportHovered) {
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

    // Viewport HUD Overlay (Camera info & quick controls)
    ImVec2 overlayPos = ImVec2(context.viewportPos.x + 10.0f,
                               context.viewportPos.y + 10.0f);
    ImGui::SetNextWindowPos(overlayPos);
    ImGui::SetNextWindowBgAlpha(0.6f);
    if (ImGui::BeginChild("ViewportHUD", ImVec2(240.0f, 65.0f), true,
                          ImGuiWindowFlags_NoDecoration |
                              ImGuiWindowFlags_NoDocking |
                              ImGuiWindowFlags_NoSavedSettings)) {
      ImGui::Text("Cam: (%.2f, %.2f) m", context.camera.center.x,
                  context.camera.center.y);
      ImGui::Text("Zoom: %.0f%%", context.camera.zoom * 100.0f);
      ImGui::SameLine();
      if (ImGui::SmallButton("Reset View")) {
        context.camera.Reset();
      }
    }
    ImGui::EndChild();
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace Elysium
