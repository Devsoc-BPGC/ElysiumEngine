#include "panels/ConsolePanel.hpp"
#include "EditorLogSink.hpp"

namespace Elysium {

ConsolePanel::ConsolePanel() : EditorPanel("Console") {}

void ConsolePanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                SimpleRenderer &renderer) {
  (void)context;
  (void)scene;
  (void)renderer;

  if (ImGui::Begin("Console", &isOpen)) {
    // Toolbar: Clear, Filter, Level toggles, Auto-scroll
    if (ImGui::Button("Clear")) {
      GetEditorLogSink()->Clear();
    }

    ImGui::SameLine();
    m_filter.Draw("Filter", 160.0f);

    ImGui::SameLine();
    ImGui::Checkbox("Trace", &m_showTrace);
    ImGui::SameLine();
    ImGui::Checkbox("Info", &m_showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warn", &m_showWarn);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &m_showError);

    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &m_autoScroll);

    ImGui::Separator();

    // Log messages scroll region
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    auto messages = GetEditorLogSink()->GetMessages();
    for (const auto &msg : messages) {
      // Filter by level
      if (msg.level == spdlog::level::trace || msg.level == spdlog::level::debug) {
        if (!m_showTrace)
          continue;
      } else if (msg.level == spdlog::level::info) {
        if (!m_showInfo)
          continue;
      } else if (msg.level == spdlog::level::warn) {
        if (!m_showWarn)
          continue;
      } else if (msg.level >= spdlog::level::err) {
        if (!m_showError)
          continue;
      }

      // Filter by text search
      if (!m_filter.PassFilter(msg.message.c_str()))
        continue;

      // Color coding based on severity
      ImVec4 color(1.0f, 1.0f, 1.0f, 1.0f);
      const char *levelPrefix = "[INFO]";

      switch (msg.level) {
      case spdlog::level::trace:
      case spdlog::level::debug:
        color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
        levelPrefix = "[TRACE]";
        break;
      case spdlog::level::info:
        color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
        levelPrefix = "[INFO]";
        break;
      case spdlog::level::warn:
        color = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
        levelPrefix = "[WARN]";
        break;
      case spdlog::level::err:
        color = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
        levelPrefix = "[ERROR]";
        break;
      case spdlog::level::critical:
        color = ImVec4(1.0f, 0.1f, 0.1f, 1.0f);
        levelPrefix = "[CRITICAL]";
        break;
      default:
        break;
      }

      ImGui::TextDisabled("[%s]", msg.timeStr.c_str());
      ImGui::SameLine();
      ImGui::TextColored(color, "%s", levelPrefix);
      ImGui::SameLine();
      ImGui::TextUnformatted(msg.message.c_str());
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
      ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
  }
  ImGui::End();
}

} // namespace Elysium
