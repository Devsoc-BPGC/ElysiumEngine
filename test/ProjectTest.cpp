#include "AssetManager.hpp"
#include "Log.h"
#include "Project.hpp"
#include "ProjectSerializer.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

void TestProjectCreationAndPaths() {
  std::filesystem::path projDir = "test_sandbox_project";
  std::error_code ec;
  std::filesystem::remove_all(projDir, ec);

  auto project = Elysium::Project::NewProject(projDir, "SandboxGame");
  assert(project != nullptr);
  assert(Elysium::Project::GetActive() == project);
  assert(project->GetConfig().Name == "SandboxGame");

  std::filesystem::path projectFilePath = projDir / "SandboxGame.elyproj";
  assert(std::filesystem::exists(projectFilePath));

  // Check directories created
  std::filesystem::path assetDir = project->GetAssetDirectory();
  assert(std::filesystem::exists(assetDir));
  assert(std::filesystem::exists(assetDir / "Scenes"));
  assert(std::filesystem::exists(assetDir / "assets.meta"));
  assert(project->GetProjectDirectory().is_absolute());

  // Check path resolution
  std::filesystem::path resolved = project->GetAssetFileSystemPath("textures/icon.png");
  assert(resolved == assetDir / "textures/icon.png");

  // Check static getters
  assert(Elysium::Project::GetActiveProjectDirectory() == project->GetProjectDirectory());
  assert(Elysium::Project::GetActiveAssetDirectory() == assetDir);
  assert(Elysium::Project::GetActiveAssetFileSystemPath("textures/icon.png") == resolved);

  std::cout << "[PASS] TestProjectCreationAndPaths" << std::endl;
}

void TestProjectSerializationAndLoading() {
  std::filesystem::path projDir = "test_sandbox_project";
  std::filesystem::path projectFilePath = projDir / "SandboxGame.elyproj";

  // Reset active project
  Elysium::Project::SetActive(nullptr);
  assert(Elysium::Project::GetActive() == nullptr);

  // Load project
  auto loadedProject = Elysium::Project::LoadProject(projectFilePath);
  assert(loadedProject != nullptr);
  assert(Elysium::Project::GetActive() == loadedProject);
  assert(loadedProject->GetConfig().Name == "SandboxGame");
  assert(loadedProject->GetConfig().AssetDirectory == "Assets");
  assert(loadedProject->GetConfig().StartScenePath == "Scenes/Default.elyscene");

  std::cout << "[PASS] TestProjectSerializationAndLoading" << std::endl;
}

void TestProjectAssetManagerIntegration() {
  std::filesystem::path projDir = "test_sandbox_project";
  auto project = Elysium::Project::GetActive();
  assert(project != nullptr);

  std::filesystem::path assetDir = project->GetAssetDirectory();

  // Create dummy texture in project asset directory
  std::filesystem::path subDir = assetDir / "Textures";
  std::filesystem::create_directories(subDir);
  std::ofstream dummyTex(subDir / "player.png");
  dummyTex << "dummy_image_data";
  dummyTex.close();

  // Reload project assets
  auto &am = Elysium::AssetManager::Get();
  am.ReloadProjectAssets(assetDir);

  // Check that player.png was registered
  assert(am.HasAsset("Textures/player.png") || am.HasAsset("Textures\\player.png"));
  Elysium::AssetHandle handle = am.GetHandleFromPath("Textures/player.png");
  if (static_cast<uint64_t>(handle) == 0) {
    handle = am.GetHandleFromPath("Textures\\player.png");
  }
  assert(static_cast<uint64_t>(handle) != 0);

  const auto &meta = am.GetMetadata(handle);
  assert(meta.type == Elysium::AssetType::Texture);

  // Manifest was saved in project asset directory
  assert(std::filesystem::exists(assetDir / "assets.meta"));

  // Cleanup test sandbox
  std::error_code ec;
  std::filesystem::remove_all(projDir, ec);

  std::cout << "[PASS] TestProjectAssetManagerIntegration" << std::endl;
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  Elysium::Log::Init();
  std::cout << "Running Elysium Project & AssetManager Integration Tests...\n";
  TestProjectCreationAndPaths();
  TestProjectSerializationAndLoading();
  TestProjectAssetManagerIntegration();
  std::cout << "\nAll Project & Integration tests PASSED successfully!\n";
  return 0;
}
