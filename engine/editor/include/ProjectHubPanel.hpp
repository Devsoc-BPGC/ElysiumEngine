#pragma once

#include "Core.hpp"
#include "RecentProjects.hpp"
#include <filesystem>
#include <functional>
#include <string>

namespace Elysium {

/**
 * @brief Parameters gathered from the Create New Project modal.
 */
struct NewProjectRequest {
  std::string projectName;
  std::filesystem::path baseDirectory;
};

/**
 * @brief Callbacks invoked by ProjectHubPanel to transition into Workspace.
 */
struct ProjectHubCallbacks {
  std::function<bool(const std::filesystem::path &projectFilePath,
                     std::string &outError)>
      onOpenProject;
  std::function<bool(const NewProjectRequest &request, std::string &outError)>
      onCreateProject;
};

/**
 * @brief Compact launcher / project hub screen displayed before entering
 *        the docking editor workspace.
 */
class ELYSIUM_API ProjectHubPanel {
public:
  explicit ProjectHubPanel(RecentProjectsManager &recentProjects,
                           ProjectHubCallbacks callbacks = {});

  void SetCallbacks(ProjectHubCallbacks callbacks) {
    m_Callbacks = std::move(callbacks);
  }

  void OnImGuiRender();

  void OpenNewProjectModal();
  void SetErrorMessage(const std::string &message);
  void ClearErrorMessage();

  static void OpenFolderInFileExplorer(const std::filesystem::path &folderPath);
  static bool IsValidProjectName(const std::string &name);
  static std::filesystem::path
  ResolveProjectFilePath(const std::filesystem::path &baseDir,
                         const std::string &projectName);

private:
  void RenderHeaderBar();
  void RenderRecentProjectsList();
  void RenderCreateNewProjectModal();

  RecentProjectsManager &m_RecentProjects;
  ProjectHubCallbacks m_Callbacks;

  char m_SearchBuffer[256] = "";

  // Create New Project modal state
  bool m_ShowCreateModal = false;
  bool m_OpenCreateModalRequested = false;
  bool m_FocusNameInput = false;
  char m_NewProjectName[128] = "MyProject";
  char m_NewProjectBaseDir[512] = "";

  // Feedback / error banner
  std::string m_ErrorMessage;
};

} // namespace Elysium
