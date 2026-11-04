#pragma once

#include "Core.hpp"
#include <filesystem>
#include <memory>
#include <string>

namespace Elysium {

/**
 * @brief Configuration data structure defining an Elysium Engine project.
 */
struct ProjectConfig {
  std::string Name = "Untitled";
  std::filesystem::path ProjectDirectory;
  std::filesystem::path AssetDirectory = "Assets"; // Relative to ProjectDirectory
  std::filesystem::path StartScenePath;            // Optional default scene path
};

/**
 * @brief Project management singleton / active context for Elysium Engine.
 *
 * Manages project configuration, filesystem paths, active project lifetime,
 * and ties directly into the Asset Manager and Editor UI systems.
 */
class ELYSIUM_API Project {
public:
  Project() = default;
  explicit Project(ProjectConfig config) : m_Config(std::move(config)) {}

  [[nodiscard]] ProjectConfig &GetConfig() { return m_Config; }
  [[nodiscard]] const ProjectConfig &GetConfig() const { return m_Config; }

  // Path resolution helpers
  [[nodiscard]] const std::filesystem::path &GetProjectDirectory() const {
    return m_Config.ProjectDirectory;
  }

  [[nodiscard]] std::filesystem::path GetAssetDirectory() const {
    if (m_Config.AssetDirectory.is_absolute()) {
      return m_Config.AssetDirectory;
    }
    return m_Config.ProjectDirectory / m_Config.AssetDirectory;
  }

  [[nodiscard]] std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path &relative) const {
    return GetAssetDirectory() / relative;
  }

  // Static Active Project Management
  static std::shared_ptr<Project> GetActive();
  static void SetActive(std::shared_ptr<Project> project);

  static std::shared_ptr<Project> NewProject(const std::filesystem::path &projectDirPath,
                                             const std::string &projectName = "NewProject");
  static std::shared_ptr<Project> LoadProject(const std::filesystem::path &projectFilePath);
  static bool SaveActiveProject();
  static bool SaveActiveProject(const std::filesystem::path &projectFilePath);

  // Helper static getters for systems
  static const std::filesystem::path &GetActiveProjectDirectory();
  static std::filesystem::path GetActiveAssetDirectory();
  static std::filesystem::path GetActiveAssetFileSystemPath(const std::filesystem::path &relative);

private:
  ProjectConfig m_Config;
  static std::shared_ptr<Project> s_ActiveProject;
};

} // namespace Elysium
