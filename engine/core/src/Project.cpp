#include "Project.hpp"
#include "ProjectSerializer.hpp"
#include "Log.h"
#include <fstream>

namespace Elysium {

std::shared_ptr<Project> Project::s_ActiveProject = nullptr;
static const std::filesystem::path s_EmptyPath;

std::shared_ptr<Project> Project::GetActive() {
  return s_ActiveProject;
}

void Project::SetActive(std::shared_ptr<Project> project) {
  s_ActiveProject = project;
}

std::shared_ptr<Project> Project::NewProject(const std::filesystem::path &projectDirPath,
                                             const std::string &projectName) {
  std::error_code ec;
  std::filesystem::path absDirPath = std::filesystem::absolute(projectDirPath).lexically_normal();

  if (!std::filesystem::exists(absDirPath, ec)) {
    std::filesystem::create_directories(absDirPath, ec);
    if (ec) {
      ELYSIUM_CORE_ERROR("Project::NewProject: Failed to create project directory '{}': {}",
                         absDirPath.string(), ec.message());
      return nullptr;
    }
  }

  // Resolve canonical path
  std::filesystem::path canonicalDir = std::filesystem::canonical(absDirPath, ec);
  if (ec) {
    canonicalDir = absDirPath;
  }

  // Set up project configuration
  ProjectConfig config;
  config.Name = projectName.empty() ? canonicalDir.stem().string() : projectName;
  config.ProjectDirectory = canonicalDir;
  config.AssetDirectory = "Assets";
  config.StartScenePath = "Scenes/Default.elyscene";

  auto project = std::make_shared<Project>(config);

  // Initialize standard folder hierarchy: Assets/, Scenes/
  std::filesystem::path assetsDir = project->GetAssetDirectory();
  std::filesystem::create_directories(assetsDir, ec);

  std::filesystem::path scenesDir = assetsDir / "Scenes";
  std::filesystem::create_directories(scenesDir, ec);

  // Initialize an empty assets.meta manifest inside Assets/
  std::filesystem::path metaPath = assetsDir / "assets.meta";
  if (!std::filesystem::exists(metaPath, ec)) {
    std::ofstream metaOut(metaPath);
    if (metaOut.is_open()) {
      metaOut << "# Elysium Engine Asset Registry Manifest\n";
      metaOut << "# Version: 1.0\n";
      metaOut << "AssetRegistry:\n";
      metaOut.close();
    }
  }

  // Save the .elyproj file
  std::filesystem::path projectFilePath = canonicalDir / (config.Name + ".elyproj");
  ProjectSerializer serializer(project);
  if (!serializer.Serialize(projectFilePath)) {
    ELYSIUM_CORE_ERROR("Project::NewProject: Failed to write project file '{}'",
                       projectFilePath.string());
    return nullptr;
  }

  SetActive(project);
  ELYSIUM_CORE_INFO("Project::NewProject: Successfully initialized project '{}' at '{}'",
                    config.Name, canonicalDir.string());
  return project;
}

std::shared_ptr<Project> Project::LoadProject(const std::filesystem::path &projectFilePath) {
  std::error_code ec;
  std::filesystem::path absFilePath = std::filesystem::absolute(projectFilePath).lexically_normal();

  if (!std::filesystem::exists(absFilePath, ec)) {
    ELYSIUM_CORE_ERROR("Project::LoadProject: Project file not found: '{}'", absFilePath.string());
    return nullptr;
  }

  std::filesystem::path canonicalFilePath = std::filesystem::canonical(absFilePath, ec);
  if (ec) {
    canonicalFilePath = absFilePath;
  }

  auto project = std::make_shared<Project>();
  ProjectSerializer serializer(project);
  if (!serializer.Deserialize(canonicalFilePath)) {
    ELYSIUM_CORE_ERROR("Project::LoadProject: Failed to deserialize project from '{}'",
                       canonicalFilePath.string());
    return nullptr;
  }

  SetActive(project);
  ELYSIUM_CORE_INFO("Project::LoadProject: Loaded project '{}' from '{}'",
                    project->GetConfig().Name, canonicalFilePath.string());
  return project;
}

bool Project::SaveActiveProject() {
  if (!s_ActiveProject) {
    ELYSIUM_CORE_WARN("Project::SaveActiveProject: No active project to save!");
    return false;
  }

  const auto &config = s_ActiveProject->GetConfig();
  std::filesystem::path projectFilePath = config.ProjectDirectory / (config.Name + ".elyproj");
  return SaveActiveProject(projectFilePath);
}

bool Project::SaveActiveProject(const std::filesystem::path &projectFilePath) {
  if (!s_ActiveProject) {
    ELYSIUM_CORE_WARN("Project::SaveActiveProject: No active project to save!");
    return false;
  }

  ProjectSerializer serializer(s_ActiveProject);
  return serializer.Serialize(projectFilePath);
}

const std::filesystem::path &Project::GetActiveProjectDirectory() {
  if (s_ActiveProject) {
    return s_ActiveProject->GetProjectDirectory();
  }
  return s_EmptyPath;
}

std::filesystem::path Project::GetActiveAssetDirectory() {
  if (s_ActiveProject) {
    return s_ActiveProject->GetAssetDirectory();
  }
  return s_EmptyPath;
}

std::filesystem::path Project::GetActiveAssetFileSystemPath(const std::filesystem::path &relative) {
  if (s_ActiveProject) {
    return s_ActiveProject->GetAssetFileSystemPath(relative);
  }
  return s_EmptyPath;
}

} // namespace Elysium
