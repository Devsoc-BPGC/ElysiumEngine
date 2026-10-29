#include "panels/GamePreviewPanel.hpp"
#include <algorithm>
#include <cmath>
#include <imgui-SFML.h>
#include <imgui.h>

namespace Elysium {

namespace {

struct AspectPreset {
  const char *label;
  float ratio; // <= 0 fills the panel instead of letterboxing
};

constexpr AspectPreset kAspectPresets[] = {
    {"16:9", 16.0f / 9.0f},
    {"16:10", 16.0f / 10.0f},
    {"4:3", 4.0f / 3.0f},
    {"Free (fill panel)", 0.0f},
};

} // namespace

GamePreviewPanel::GamePreviewPanel() : EditorPanel("Game Preview") {}

bool GamePreviewPanel::EnsureTarget(unsigned int width, unsigned int height) {
  if (m_currentTextureSize.x == width && m_currentTextureSize.y == height)
    return true;

  if (!m_previewTexture.resize(sf::Vector2u(width, height))) {
    // Don't trust the old target after a failed resize; retry next frame
    m_currentTextureSize = {0, 0};
    return false;
  }
  m_currentTextureSize = sf::Vector2u(width, height);
  return true;
}

void GamePreviewPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                     SimpleRenderer &renderer) {
  // The default layout docks the preview as a tab beside the scene viewport.
  // Do the same when no saved layout places it, e.g. a project's imgui.ini
  // from before the Game Preview existed. The scene viewport renders earlier
  // in the frame, so its dock node is current.
  if (context.viewportDockId != 0)
    ImGui::SetNextWindowDockID(context.viewportDockId, ImGuiCond_FirstUseEver);

  if (m_focusRequested) {
    ImGui::SetNextWindowCollapsed(false);
    ImGui::SetNextWindowFocus();
    m_focusRequested = false;
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::Begin("Game Preview", &isOpen,
                   ImGuiWindowFlags_NoScrollbar |
                       ImGuiWindowFlags_NoScrollWithMouse)) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    GameCamera &cam = context.gameCamera;

    // Largest rect of the game's aspect ratio that fits the panel
    float targetW = std::max(1.0f, avail.x);
    float targetH = std::max(1.0f, avail.y);
    if (cam.aspectRatio > 0.0f) {
      if (targetW / targetH > cam.aspectRatio)
        targetW = targetH * cam.aspectRatio;
      else
        targetH = targetW / cam.aspectRatio;
    }
    unsigned int width = static_cast<unsigned int>(std::floor(targetW));
    unsigned int height = static_cast<unsigned int>(std::floor(targetH));

    if (avail.x >= 1.0f && avail.y >= 1.0f && width > 0 && height > 0 &&
        EnsureTarget(width, height)) {
      // The game camera fixes how much world is visible, so resizing the
      // panel scales the picture instead of revealing more of the scene
      float ptm = context.pixelsPerMeter;
      float outputAspect = static_cast<float>(width) / static_cast<float>(height);
      sf::View view;
      view.setCenter({cam.center.x * ptm, cam.center.y * ptm});
      view.setSize({cam.viewHeight * outputAspect * ptm, cam.viewHeight * ptm});

      // The renderer is shared with the scene viewport, and the Stats panel
      // reads its counters directly, so leave it exactly as we found it
      const SimpleRenderer savedRenderer = renderer;
      renderer.drawDebugGrid = context.gamePreviewDebugVisuals;
      renderer.drawPhysicsColliders = context.gamePreviewDebugVisuals;
      renderer.drawAABBs = context.gamePreviewDebugVisuals;
      renderer.pixelsPerMeter = ptm;
      // While playing, show the simulated copy rather than the edit scene
      Scene &shown = context.runtimeScene ? *context.runtimeScene : scene;
      renderer.Render(m_previewTexture, shown, view);
      m_previewTexture.display();
      renderer = savedRenderer;

      // Centre the image; the bands around it are the letterbox
      ImVec2 imagePos(origin.x + (avail.x - static_cast<float>(width)) * 0.5f,
                      origin.y + (avail.y - static_cast<float>(height)) * 0.5f);
      ImGui::SetCursorScreenPos(imagePos);
      ImGui::Image(m_previewTexture);

      // Outside Play mode the frame is edit-time or frozen state, so say so
      if (context.playState != ScenePlayState::Play) {
        const char *label = context.playState == ScenePlayState::Edit
                                ? "NOT PLAYING"
                                : "PAUSED";
        ImGui::SetCursorScreenPos(
            ImVec2(imagePos.x + 8.0f, imagePos.y + 6.0f));
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "%s", label);
      }
    }

    // Right-click: game output settings. Nothing here touches the editor
    // camera.
    if (ImGui::BeginPopupContextWindow("GamePreviewSettings")) {
      ImGui::TextDisabled("Aspect Ratio");
      for (const AspectPreset &preset : kAspectPresets) {
        bool selected = std::fabs(cam.aspectRatio - preset.ratio) < 0.001f;
        if (ImGui::MenuItem(preset.label, nullptr, selected)) {
          cam.aspectRatio = preset.ratio;
        }
      }
      ImGui::Separator();
      ImGui::TextDisabled("Game Camera");
      float center[2] = {cam.center.x, cam.center.y};
      if (ImGui::DragFloat2("Center (m)", center, 0.05f)) {
        cam.center = {center[0], center[1]};
      }
      ImGui::SliderFloat("View Height (m)", &cam.viewHeight, 2.0f, 60.0f,
                         "%.1f");
      if (ImGui::MenuItem("Reset Game Camera")) {
        cam.Reset();
      }
      ImGui::Separator();
      ImGui::MenuItem("Show Debug Visuals", nullptr,
                      &context.gamePreviewDebugVisuals);
      ImGui::EndPopup();
    }
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace Elysium
