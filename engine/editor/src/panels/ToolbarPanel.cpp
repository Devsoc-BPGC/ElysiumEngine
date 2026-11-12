#include "elypch.hpp"
#include "panels/ToolbarPanel.hpp"

namespace Elysium {

ToolbarPanel::ToolbarPanel() : EditorPanel("Toolbar") {}

void ToolbarPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                 SimpleRenderer &renderer) {
  (void)scene;
  (void)renderer;

  ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoScrollWithMouse |
                           ImGuiWindowFlags_NoTitleBar;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 6.0f));
  if (ImGui::Begin("##Toolbar", nullptr, flags)) {
    // Mode status indicator
    if (context.playState == ScenePlayState::Edit) {
      ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[EDIT MODE]");
    } else if (context.playState == ScenePlayState::Play) {
      ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "[PLAYING]");
    } else if (context.playState == ScenePlayState::Pause) {
      ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[PAUSED]");
    } else if (context.playState == ScenePlayState::Step) {
      ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "[STEPPING]");
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Gizmo Tool Buttons: Move (G), Rotate (R), Scale (S)
    auto DrawToolButton = [&](const char *label, GizmoOperation op) {
      bool isActive = (context.gizmoOperation == op);
      if (isActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.55f, 0.9f, 1.0f));
      }
      if (ImGui::Button(label)) {
        context.gizmoOperation = isActive ? GizmoOperation::None : op;
      }
      if (isActive) {
        ImGui::PopStyleColor(2);
      }
    };

    DrawToolButton("Move (G)", GizmoOperation::Translate);
    ImGui::SameLine();
    DrawToolButton("Rotate (R)", GizmoOperation::Rotate);
    ImGui::SameLine();
    DrawToolButton("Scale (S)", GizmoOperation::Scale);
    ImGui::SameLine();

    // Coordinate Space: World / Local (W)
    const char *spaceLabel = (context.gizmoMode == GizmoMode::World) ? "[World] (W)" : "[Local] (W)";
    if (ImGui::Button(spaceLabel)) {
      context.gizmoMode = (context.gizmoMode == GizmoMode::World) ? GizmoMode::Local : GizmoMode::World;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Toggle Gizmo Reference Space (World vs. Local) [Hotkey: W]");
    }
    ImGui::SameLine();

    // Dedicated Snap Toggle Button (E) + Settings Popup
    bool snapOn = context.snapEnabled;
    if (snapOn) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.3f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.65f, 0.38f, 1.0f));
    }
    if (ImGui::Button(snapOn ? "Snap: ON (E)" : "Snap: OFF (E)")) {
      context.snapEnabled = !context.snapEnabled;
    }
    if (snapOn) {
      ImGui::PopStyleColor(2);
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Toggle Grid/Angle/Scale Snapping [Hotkey: E]\n(Hold Ctrl while dragging to temporarily snap)");
    }
    ImGui::SameLine();
    if (ImGui::Button("...##SnapSettings")) {
      ImGui::OpenPopup("SnapSettingsPopup");
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Configure Snap Steps");
    }

    if (ImGui::BeginPopup("SnapSettingsPopup")) {
      ImGui::TextDisabled("Snap Step Configuration");
      ImGui::Separator();
      ImGui::DragFloat("Move Step (m)", &context.snapTranslation, 0.05f, 0.05f, 10.0f, "%.2f m");
      ImGui::DragFloat("Rotate Step", &context.snapRotation, 1.0f, 1.0f, 90.0f, "%.1f deg");
      ImGui::DragFloat("Scale Step", &context.snapScale, 0.01f, 0.01f, 2.0f, "%.2f");
      ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    // Center-aligned simulation playback buttons
    float buttonWidth = 80.0f;
    float totalControlsWidth = buttonWidth * 3.0f + 30.0f;
    float availWidth = ImGui::GetContentRegionAvail().x;
    if (availWidth > totalControlsWidth) {
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                           (availWidth - totalControlsWidth) * 0.5f);
    }

    // Play / Stop button
    if (context.playState == ScenePlayState::Edit) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.6f, 0.25f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(0.2f, 0.75f, 0.35f, 1.0f));
      if (ImGui::Button(" Play ", ImVec2(buttonWidth, 0))) {
        context.playState = ScenePlayState::Play;
        ELYSIUM_INFO("Editor: Switched to PLAY mode");
      }
      ImGui::PopStyleColor(2);
    } else {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
      if (ImGui::Button(" Stop ", ImVec2(buttonWidth, 0))) {
        context.playState = ScenePlayState::Edit;
        ELYSIUM_INFO("Editor: Switched to EDIT mode");
      }
      ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();

    // Pause / Resume button
    if (context.playState == ScenePlayState::Pause) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.55f, 0.1f, 1.0f));
      if (ImGui::Button(" Resume ", ImVec2(buttonWidth, 0))) {
        context.playState = ScenePlayState::Play;
        ELYSIUM_INFO("Editor: Resumed simulation");
      }
      ImGui::PopStyleColor();
    } else {
      bool isPlay = (context.playState == ScenePlayState::Play);
      if (!isPlay) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Button(" Pause ", ImVec2(buttonWidth, 0))) {
        context.playState = ScenePlayState::Pause;
        ELYSIUM_INFO("Editor: Paused simulation");
      }
      if (!isPlay) {
        ImGui::EndDisabled();
      }
    }

    ImGui::SameLine();

    // Step button
    bool canStep = (context.playState == ScenePlayState::Edit ||
                    context.playState == ScenePlayState::Pause);
    if (!canStep) {
      ImGui::BeginDisabled();
    }
    if (ImGui::Button(" Step ", ImVec2(buttonWidth, 0))) {
      context.playState = ScenePlayState::Step;
      ELYSIUM_INFO("Editor: Single step frame executed");
    }
    if (!canStep) {
      ImGui::EndDisabled();
    }
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace Elysium
