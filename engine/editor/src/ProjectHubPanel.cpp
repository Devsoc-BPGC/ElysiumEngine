#include "elypch.hpp"
#include "ProjectHubPanel.hpp"
#include "FileDialog.hpp"
#include "Project.hpp"
#include "ProjectSerializer.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

#if defined(ELYSIUM_PLATFORM_WINDOWS) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

namespace Elysium {

namespace {

std::string ToLowerCopy(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

void CopyStringToBuffer(const std::string &src, char *dst, std::size_t dstSize) {
  if (!dst || dstSize == 0) {
    return;
  }
  std::strncpy(dst, src.c_str(), dstSize - 1);
  dst[dstSize - 1] = '\0';
}

std::string EscapeShellQuotes(const std::string &input) {
  std::string escaped;
  escaped.reserve(input.size());
  for (char c : input) {
    if (c == '"' || c == '\\') {
      escaped.push_back('\\');
    }
    escaped.push_back(c);
  }
  return escaped;
}

std::string TruncateWithEllipsis(const std::string &text, float maxWidth) {
  if (maxWidth <= 0.0f) {
    return "";
  }
  if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) {
    return text;
  }
  std::string result = text;
  while (result.size() > 3 &&
         ImGui::CalcTextSize((result + "...").c_str()).x > maxWidth) {
    result.pop_back();
  }
  return result + "...";
}

} // namespace

ProjectHubPanel::ProjectHubPanel(RecentProjectsManager &recentProjects,
                                 ProjectHubCallbacks callbacks)
    : m_RecentProjects(recentProjects), m_Callbacks(std::move(callbacks)) {
  std::error_code ec;
  std::filesystem::path defaultBase =
      (std::filesystem::current_path(ec) / "projects").lexically_normal();
  CopyStringToBuffer(defaultBase.string(), m_NewProjectBaseDir,
                     sizeof(m_NewProjectBaseDir));
}

void ProjectHubPanel::OpenNewProjectModal() {
  m_ShowCreateModal = true;
  m_OpenCreateModalRequested = true;
  m_FocusNameInput = true;
}

void ProjectHubPanel::SetErrorMessage(const std::string &message) {
  m_ErrorMessage = message;
}

void ProjectHubPanel::ClearErrorMessage() { m_ErrorMessage.clear(); }

bool ProjectHubPanel::IsValidProjectName(const std::string &name) {
  if (name.empty() || name == "." || name == "..") {
    return false;
  }
  const std::string invalidChars = "/\\:*?\"<>|";
  for (char c : name) {
    if (static_cast<unsigned char>(c) < 32 ||
        invalidChars.find(c) != std::string::npos) {
      return false;
    }
  }
  if (std::isspace(static_cast<unsigned char>(name.front())) ||
      std::isspace(static_cast<unsigned char>(name.back()))) {
    return false;
  }
  return true;
}

std::filesystem::path ProjectHubPanel::ResolveProjectFilePath(
    const std::filesystem::path &baseDir, const std::string &projectName) {
  if (baseDir.empty() || projectName.empty()) {
    return {};
  }
  return (baseDir / projectName / (projectName + ".elyproj"))
      .lexically_normal();
}

void ProjectHubPanel::OpenFolderInFileExplorer(
    const std::filesystem::path &folderPath) {
  if (folderPath.empty()) {
    return;
  }

  std::error_code ec;
  std::filesystem::path target = folderPath;
  if (std::filesystem::is_regular_file(target, ec)) {
    target = target.parent_path();
  }
  if (!std::filesystem::exists(target, ec)) {
    ELYSIUM_CORE_WARN("ProjectHub: Directory does not exist: '{}'",
                      target.string());
    return;
  }

#if defined(ELYSIUM_PLATFORM_WINDOWS) || defined(_WIN32)
  ShellExecuteA(nullptr, "open", target.string().c_str(), nullptr, nullptr,
                SW_SHOWNORMAL);
#elif defined(ELYSIUM_PLATFORM_MAC) || defined(__APPLE__)
  const std::string cmd =
      "open \"" + EscapeShellQuotes(target.string()) + "\" &";
  (void)std::system(cmd.c_str());
#elif defined(ELYSIUM_PLATFORM_LINUX) || defined(__linux__)
  const std::string cmd = "xdg-open \"" + EscapeShellQuotes(target.string()) +
                          "\" >/dev/null 2>&1 &";
  (void)std::system(cmd.c_str());
#else
  ELYSIUM_CORE_WARN(
      "ProjectHub: Opening file explorer is not supported on this platform");
#endif
}

void ProjectHubPanel::OnImGuiRender() {
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  const ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
      ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  if (ImGui::Begin("ElysiumProjectHub", nullptr, windowFlags)) {
    ImGui::PopStyleVar(3);

    const float availW = ImGui::GetContentRegionAvail().x;
    const float availH = ImGui::GetContentRegionAvail().y;
    const float horizontalMargin = 20.0f;
    const float verticalMargin = 16.0f;
    const float contentWidth = std::max(280.0f, availW - 2.0f * horizontalMargin);
    const float contentHeight = std::max(180.0f, availH - 2.0f * verticalMargin);

    ImGui::SetCursorPos(ImVec2(horizontalMargin, verticalMargin));
    ImGui::BeginChild("ProjectHubContent",
                      ImVec2(contentWidth, contentHeight), false,
                      ImGuiWindowFlags_NoScrollbar);

    RenderHeaderBar();

    if (!m_ErrorMessage.empty()) {
      ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.32f, 0.12f, 0.14f, 0.90f));
      ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.80f, 0.30f, 0.32f, 0.90f));
      ImGui::BeginChild("HubErrorBanner", ImVec2(0.0f, 40.0f), true,
                        ImGuiWindowFlags_NoScrollbar);
      ImGui::AlignTextToFramePadding();
      ImGui::TextColored(ImVec4(1.0f, 0.78f, 0.78f, 1.0f), "%s",
                         m_ErrorMessage.c_str());
      const float dismissW = 76.0f;
      const float bannerAvail = ImGui::GetContentRegionAvail().x;
      if (bannerAvail > dismissW + 8.0f) {
        ImGui::SameLine(ImGui::GetCursorPosX() + bannerAvail - dismissW);
      } else {
        ImGui::SameLine();
      }
      if (ImGui::Button("Dismiss", ImVec2(dismissW, 0.0f))) {
        ClearErrorMessage();
      }
      ImGui::EndChild();
      ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();
    RenderRecentProjectsList();

    ImGui::EndChild();

    RenderCreateNewProjectModal();
  } else {
    ImGui::PopStyleVar(3);
  }
  ImGui::End();
}

void ProjectHubPanel::RenderHeaderBar() {
  ImGui::BeginGroup();
  ImGui::SetWindowFontScale(1.25f);
  ImGui::TextColored(ImVec4(0.42f, 0.72f, 1.0f, 1.0f), "ELYSIUM ENGINE");
  ImGui::SetWindowFontScale(1.0f);
  ImGui::SameLine();
  ImGui::AlignTextToFramePadding();
  ImGui::TextDisabled("  |  Project Hub");
  ImGui::TextDisabled(
      "Open a recent project, browse for a .elyproj, or create a new project.");
  ImGui::EndGroup();

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  const float totalW = ImGui::GetContentRegionAvail().x;
  const float openBtnW = 132.0f;
  const float newBtnW = 136.0f;
  const float clearBtnW = (m_SearchBuffer[0] != '\0') ? 26.0f : 0.0f;
  const float spacing = ImGui::GetStyle().ItemSpacing.x;
  const float buttonsTotalW =
      openBtnW + newBtnW + spacing + (clearBtnW > 0.0f ? clearBtnW + spacing : 0.0f);
  const float searchW = std::max(140.0f, totalW - buttonsTotalW - spacing * 2.0f);

  ImGui::SetNextItemWidth(searchW);
  ImGui::InputTextWithHint("##SearchRecentProjects",
                           "Search recent projects...", m_SearchBuffer,
                           sizeof(m_SearchBuffer));

  if (m_SearchBuffer[0] != '\0') {
    ImGui::SameLine();
    if (ImGui::Button("X##ClearSearch", ImVec2(clearBtnW, 0.0f))) {
      m_SearchBuffer[0] = '\0';
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Clear search filter");
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("Open Project...", ImVec2(openBtnW, 0.0f))) {
    const std::vector<FileDialogFilter> filters = {
        {"Elysium Project (*.elyproj)", "*.elyproj"}};
    auto selected = FileDialog::OpenFile(
        "Open Elysium Project", std::filesystem::current_path(), filters);
    if (selected.has_value() && m_Callbacks.onOpenProject) {
      std::string err;
      if (!m_Callbacks.onOpenProject(*selected, err)) {
        SetErrorMessage(err.empty() ? "Failed to open project file." : err);
      }
    }
  }

  ImGui::SameLine();
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.82f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4(0.24f, 0.55f, 0.94f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                        ImVec4(0.16f, 0.40f, 0.74f, 1.0f));
  if (ImGui::Button("+ New Project", ImVec2(newBtnW, 0.0f))) {
    OpenNewProjectModal();
  }
  ImGui::PopStyleColor(3);
}

void ProjectHubPanel::RenderRecentProjectsList() {
  const auto &entries = m_RecentProjects.GetEntries();
  const bool hasMissing =
      std::any_of(entries.begin(), entries.end(),
                  [](const RecentProjectEntry &e) { return !e.exists; });

  ImGui::AlignTextToFramePadding();
  ImGui::Text("Recent Projects (%zu)", entries.size());
  if (hasMissing) {
    const float clearMissingW = 120.0f;
    const float avail = ImGui::GetContentRegionAvail().x;
    if (avail > clearMissingW) {
      ImGui::SameLine(ImGui::GetCursorPosX() + avail - clearMissingW);
    } else {
      ImGui::SameLine();
    }
    if (ImGui::SmallButton("Clear Missing##ClearMissingBtn")) {
      m_RecentProjects.ClearMissing();
    }
  }

  ImGui::Spacing();

  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.10f, 0.12f, 0.85f));
  ImGui::BeginChild("RecentProjectsScroll", ImVec2(0.0f, 0.0f), true);

  const std::string queryLower = ToLowerCopy(m_SearchBuffer);

  std::vector<std::size_t> visibleIndices;
  visibleIndices.reserve(entries.size());
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (!queryLower.empty()) {
      const std::string nameLower = ToLowerCopy(entries[i].name);
      const std::string pathLower =
          ToLowerCopy(entries[i].projectFilePath.generic_string());
      if (nameLower.find(queryLower) == std::string::npos &&
          pathLower.find(queryLower) == std::string::npos) {
        continue;
      }
    }
    visibleIndices.push_back(i);
  }

  if (entries.empty()) {
    const ImVec2 region = ImGui::GetContentRegionAvail();
    const float centerY = std::max(24.0f, region.y * 0.35f);
    ImGui::SetCursorPosY(centerY);

    const char *msg = "No recent projects yet.";
    const char *hint =
        "Create a new project or open an existing .elyproj file.";
    const float msgW = ImGui::CalcTextSize(msg).x;
    const float hintW = ImGui::CalcTextSize(hint).x;

    ImGui::SetCursorPosX(std::max(16.0f, (region.x - msgW) * 0.5f));
    ImGui::Text("%s", msg);
    ImGui::Spacing();
    ImGui::SetCursorPosX(std::max(16.0f, (region.x - hintW) * 0.5f));
    ImGui::TextDisabled("%s", hint);
  } else if (visibleIndices.empty()) {
    const ImVec2 region = ImGui::GetContentRegionAvail();
    const char *msg = "No recent projects match your search.";
    const float msgW = ImGui::CalcTextSize(msg).x;
    ImGui::SetCursorPos(ImVec2(std::max(16.0f, (region.x - msgW) * 0.5f),
                               std::max(24.0f, region.y * 0.35f)));
    ImGui::TextDisabled("%s", msg);
  } else {
    std::optional<std::filesystem::path> pendingOpenPath;
    std::optional<std::filesystem::path> pendingRemovePath;
    std::optional<std::filesystem::path> pendingLocateOldPath;
    std::optional<std::filesystem::path> pendingExplorerPath;

    const float cardHeight = 64.0f;
    const float actionAreaWidth = 216.0f;

    for (std::size_t idx : visibleIndices) {
      const RecentProjectEntry &entry = entries[idx];
      const std::string pathStr = entry.projectFilePath.generic_string();

      ImGui::PushID(static_cast<int>(idx));

      const ImVec2 cardMin = ImGui::GetCursorScreenPos();
      const float cardWidth = ImGui::GetContentRegionAvail().x;
      const ImVec2 cardMax(cardMin.x + cardWidth, cardMin.y + cardHeight);
      const float clickableWidth = std::max(100.0f, cardWidth - actionAreaWidth);

      ImDrawList *drawList = ImGui::GetWindowDrawList();

      const bool clickedCard = ImGui::InvisibleButton(
          "##ProjectCardHitbox", ImVec2(clickableWidth, cardHeight));
      const bool cardHovered = ImGui::IsItemHovered();

      if (ImGui::BeginPopupContextItem("##ProjectCardContextMenu")) {
        if (ImGui::MenuItem("Open Project", nullptr, false, entry.exists)) {
          pendingOpenPath = entry.projectFilePath;
        }
        if (ImGui::MenuItem(entry.exists ? "Relocate..." : "Locate...")) {
          pendingLocateOldPath = entry.projectFilePath;
        }
        std::error_code ec;
        const bool parentExists =
            std::filesystem::exists(entry.projectFilePath.parent_path(), ec);
        if (ImGui::MenuItem("Open Folder in Explorer", nullptr, false,
                            parentExists)) {
          pendingExplorerPath = entry.projectFilePath.parent_path();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Remove from Recents")) {
          pendingRemovePath = entry.projectFilePath;
        }
        ImGui::EndPopup();
      }

      if (clickedCard && entry.exists) {
        pendingOpenPath = entry.projectFilePath;
      }

      const bool rowHovered =
          cardHovered || ImGui::IsMouseHoveringRect(cardMin, cardMax, true);

      ImU32 bgCol = IM_COL32(30, 32, 40, 230);
      ImU32 borderCol = IM_COL32(55, 59, 72, 170);
      if (!entry.exists) {
        bgCol = rowHovered ? IM_COL32(54, 32, 35, 240)
                           : IM_COL32(44, 27, 30, 220);
        borderCol = IM_COL32(195, 75, 78, 190);
      } else if (cardHovered) {
        bgCol = IM_COL32(42, 48, 62, 245);
        borderCol = IM_COL32(82, 145, 235, 220);
      } else if (rowHovered) {
        bgCol = IM_COL32(36, 40, 50, 235);
      }

      drawList->AddRectFilled(cardMin, cardMax, bgCol, 6.0f);
      drawList->AddRect(cardMin, cardMax, borderCol, 6.0f, 0, 1.0f);

      if (cardHovered) {
        if (entry.exists) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
          ImGui::SetTooltip("Click to open '%s'\n%s", entry.name.c_str(),
                            pathStr.c_str());
        } else {
          ImGui::SetTooltip(
              "Project file is missing on disk:\n%s\nUse Locate... or Remove.",
              pathStr.c_str());
        }
      }

      const float textLeft = cardMin.x + 12.0f;
      const float maxTextWidth = std::max(80.0f, clickableWidth - 20.0f);

      ImGui::SetCursorScreenPos(ImVec2(textLeft, cardMin.y + 9.0f));
      if (entry.exists) {
        ImGui::TextColored(ImVec4(0.95f, 0.96f, 0.99f, 1.0f), "%s",
                           entry.name.c_str());
      } else {
        ImGui::TextColored(ImVec4(0.92f, 0.68f, 0.68f, 1.0f), "%s",
                           entry.name.c_str());
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.98f, 0.40f, 0.42f, 1.0f), "[Missing]");
      }

      const std::string displayPath =
          TruncateWithEllipsis(pathStr, maxTextWidth);
      ImGui::SetCursorScreenPos(ImVec2(textLeft, cardMin.y + 34.0f));
      ImGui::TextDisabled("%s", displayPath.c_str());

      const float rightSectionX = cardMax.x - actionAreaWidth + 6.0f;
      ImGui::SetCursorScreenPos(ImVec2(rightSectionX, cardMin.y + 9.0f));
      ImGui::TextDisabled("%s", entry.lastOpenedDisplay.c_str());

      const float btnY = cardMin.y + 32.0f;
      if (!entry.exists) {
        ImGui::SetCursorScreenPos(ImVec2(rightSectionX, btnY));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.46f, 0.78f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.30f, 0.56f, 0.90f, 1.0f));
        if (ImGui::SmallButton("Locate...")) {
          pendingLocateOldPath = entry.projectFilePath;
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.20f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.72f, 0.26f, 0.28f, 1.0f));
        if (ImGui::SmallButton("Remove")) {
          pendingRemovePath = entry.projectFilePath;
        }
        ImGui::PopStyleColor(2);
      } else {
        ImGui::SetCursorScreenPos(ImVec2(rightSectionX, btnY));
        if (ImGui::SmallButton("Open Folder")) {
          pendingExplorerPath = entry.projectFilePath.parent_path();
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Open project folder in OS file explorer");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove")) {
          pendingRemovePath = entry.projectFilePath;
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Remove from recent projects list");
        }
      }

      ImGui::SetCursorScreenPos(ImVec2(cardMin.x, cardMax.y + 6.0f));
      ImGui::Dummy(ImVec2(0.0f, 0.0f));

      ImGui::PopID();
    }

    if (pendingExplorerPath.has_value()) {
      OpenFolderInFileExplorer(*pendingExplorerPath);
    }
    if (pendingRemovePath.has_value()) {
      m_RecentProjects.RemoveProject(*pendingRemovePath);
    }
    if (pendingLocateOldPath.has_value()) {
      const std::vector<FileDialogFilter> filters = {
          {"Elysium Project (*.elyproj)", "*.elyproj"}};
      const std::filesystem::path startDir =
          std::filesystem::exists(pendingLocateOldPath->parent_path())
              ? pendingLocateOldPath->parent_path()
              : std::filesystem::current_path();
      auto located =
          FileDialog::OpenFile("Locate Elysium Project", startDir, filters);
      if (located.has_value()) {
        auto proj = std::make_shared<Project>();
        ProjectSerializer serializer(proj);
        if (serializer.Deserialize(*located)) {
          m_RecentProjects.RelocateProject(*pendingLocateOldPath, *located,
                                           proj->GetConfig().Name);
          ClearErrorMessage();
        } else {
          SetErrorMessage("Selected file could not be read as a valid .elyproj.");
        }
      }
    }
    if (pendingOpenPath.has_value() && m_Callbacks.onOpenProject) {
      std::string err;
      if (!m_Callbacks.onOpenProject(*pendingOpenPath, err)) {
        SetErrorMessage(err.empty() ? "Failed to open selected project." : err);
        m_RecentProjects.ValidateEntries();
      }
    }
  }

  ImGui::EndChild();
  ImGui::PopStyleColor();
}

void ProjectHubPanel::RenderCreateNewProjectModal() {
  if (m_OpenCreateModalRequested) {
    ImGui::OpenPopup("Create New Project");
    m_OpenCreateModalRequested = false;
  }

  const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(540.0f, 0.0f), ImGuiCond_Appearing);

  if (ImGui::BeginPopupModal("Create New Project", &m_ShowCreateModal,
                             ImGuiWindowFlags_AlwaysAutoResize |
                                 ImGuiWindowFlags_NoMove)) {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      m_ShowCreateModal = false;
      ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
      return;
    }

    ImGui::Text("Create a new Elysium Engine project.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (m_FocusNameInput) {
      ImGui::SetKeyboardFocusHere();
      m_FocusNameInput = false;
    }

    ImGui::Text("Project Name");
    ImGui::SetNextItemWidth(490.0f);
    ImGui::InputText("##NewProjectName", m_NewProjectName,
                     sizeof(m_NewProjectName));

    ImGui::Spacing();
    ImGui::Text("Base Directory");
    ImGui::SetNextItemWidth(382.0f);
    ImGui::InputText("##NewProjectBaseDir", m_NewProjectBaseDir,
                     sizeof(m_NewProjectBaseDir));
    ImGui::SameLine();
    if (ImGui::Button("Browse...##BrowseBaseDir", ImVec2(100.0f, 0.0f))) {
      auto selectedDir = FileDialog::SelectFolder(
          "Select Base Directory", std::filesystem::path(m_NewProjectBaseDir));
      if (selectedDir.has_value()) {
        CopyStringToBuffer(selectedDir->string(), m_NewProjectBaseDir,
                           sizeof(m_NewProjectBaseDir));
      }
    }

    const std::string projName(m_NewProjectName);
    const std::filesystem::path baseDir(m_NewProjectBaseDir);
    const std::filesystem::path resolvedProjFile =
        ResolveProjectFilePath(baseDir, projName);

    ImGui::Spacing();
    ImGui::TextDisabled("Project File Preview:");
    if (!resolvedProjFile.empty()) {
      ImGui::TextColored(ImVec4(0.45f, 0.78f, 1.0f, 1.0f), "%s",
                         resolvedProjFile.generic_string().c_str());
    } else {
      ImGui::TextDisabled("<BaseDir>/<ProjectName>/<ProjectName>.elyproj");
    }
    ImGui::TextDisabled(
        "Creates: Assets/{Scenes, Textures, Audio, Scripts} and Main.elyscene");

    std::string validationError;
    std::error_code ec;
    if (!IsValidProjectName(projName)) {
      validationError =
          "Enter a valid project name (no slashes, quotes, or trailing spaces).";
    } else if (baseDir.empty()) {
      validationError = "Base directory cannot be empty.";
    } else if (std::filesystem::exists(resolvedProjFile, ec)) {
      validationError =
          "A project file already exists at this location.";
    }

    ImGui::Spacing();
    if (!validationError.empty()) {
      ImGui::TextColored(ImVec4(0.95f, 0.42f, 0.42f, 1.0f), "%s",
                         validationError.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const bool canCreate = validationError.empty();
    if (!canCreate) {
      ImGui::BeginDisabled();
    }
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.82f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.24f, 0.55f, 0.94f, 1.0f));
    if (ImGui::Button("Create Project", ImVec2(140.0f, 0.0f))) {
      NewProjectRequest request;
      request.projectName = projName;
      request.baseDirectory = baseDir;

      if (m_Callbacks.onCreateProject) {
        std::string err;
        if (m_Callbacks.onCreateProject(request, err)) {
          m_ShowCreateModal = false;
          ClearErrorMessage();
          ImGui::CloseCurrentPopup();
        } else {
          SetErrorMessage(err.empty() ? "Failed to create project." : err);
        }
      }
    }
    ImGui::PopStyleColor(2);
    if (!canCreate) {
      ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f))) {
      m_ShowCreateModal = false;
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

} // namespace Elysium
