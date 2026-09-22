#include "panels/StatsPanel.hpp"
#include <imgui.h>

namespace Elysium {

StatsPanel::StatsPanel() : EditorPanel("Stats & Profiler") {
  m_fpsHistory.fill(60.0f);
}

void StatsPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                              SimpleRenderer &renderer) {
  if (ImGui::Begin("Stats & Profiler", &isOpen)) {
    // Record current FPS into rolling history buffer
    m_fpsHistory[m_historyIndex] = context.stats.fps;
    m_historyIndex = (m_historyIndex + 1) % m_fpsHistory.size();

    // 1. Performance Overview
    if (ImGui::CollapsingHeader("Frame Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("FPS: %.1f", context.stats.fps);
      ImGui::Text("Frame Time: %.2f ms", context.stats.frameTimeMs);

      char overlay[32];
      snprintf(overlay, sizeof(overlay), "FPS: %.1f", context.stats.fps);
      ImGui::PlotLines("##FPSHistory", m_fpsHistory.data(),
                       static_cast<int>(m_fpsHistory.size()),
                       static_cast<int>(m_historyIndex), overlay, 0.0f, 120.0f,
                       ImVec2(ImGui::GetContentRegionAvail().x, 60.0f));
    }

    // 2. Timing Breakdown (Update, Physics, Render)
    if (ImGui::CollapsingHeader("Frame Timing Breakdown", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Update Subsystem:  %.3f ms", context.stats.updateTimeMs);
      ImGui::Text("Physics World:     %.3f ms", context.stats.physicsTimeMs);
      ImGui::Text("Renderer Pipeline: %.3f ms", context.stats.renderTimeMs);

      float total = context.stats.updateTimeMs + context.stats.physicsTimeMs +
                    context.stats.renderTimeMs;
      if (total > 0.0f) {
        ImGui::ProgressBar(context.stats.physicsTimeMs / total, ImVec2(-1.0f, 0.0f),
                           "Physics vs Frame");
      }
    }

    // 3. Scene & Render Metrics
    if (ImGui::CollapsingHeader("Scene & Rendering Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Text("Active Entities: %u", context.stats.entityCount);
      ImGui::Text("Draw Calls:      %u", renderer.drawCalls);
      ImGui::Text("Primitives:      %u", renderer.primitivesCount);
      ImGui::Text("Viewport Size:   %.0f x %.0f", context.viewportSize.x,
                  context.viewportSize.y);
    }

    // 4. Viewport & Physics Debug Options
    if (ImGui::CollapsingHeader("Viewport & Debug Options", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::Checkbox("Show Debug Grid", &context.drawDebugGrid);
      ImGui::Checkbox("Show Physics Colliders", &context.drawPhysicsColliders);
      ImGui::Checkbox("Show AABB Bounds", &context.drawAABBs);
      ImGui::SliderFloat("Pixels Per Meter", &context.pixelsPerMeter, 10.0f, 150.0f, "%.1f");
      ImGui::Text("Boundary Rotation: %.2f deg",
                  scene.physicsWorld.boundaryRotation * 180.0f / 3.14159265f);
    }
  }
  ImGui::End();
}

} // namespace Elysium
