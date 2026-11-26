#include "Log.h"
#include "RecentProjects.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace Elysium;

namespace {

void Check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[FAIL] " << message << std::endl;
    std::exit(1);
  }
  std::cout << "[PASS] " << message << std::endl;
}

void TouchFile(const std::filesystem::path &path) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path);
  out << "# Dummy .elyproj\n";
}

} // namespace

void TestPersistenceAndSorting(const std::filesystem::path &sandbox) {
  const std::filesystem::path storageFile = sandbox / "recent_projects.txt";
  const std::filesystem::path alphaProj = sandbox / "Alpha" / "Alpha.elyproj";
  const std::filesystem::path betaProj = sandbox / "Beta" / "Beta.elyproj";
  TouchFile(alphaProj);
  TouchFile(betaProj);

  {
    RecentProjectsManager manager(storageFile);
    manager.AddOrUpdateProject("Alpha", alphaProj, 1700000000);
    manager.AddOrUpdateProject("Beta", betaProj, 1700001000);
  }

  RecentProjectsManager reloaded(storageFile);
  reloaded.Load();

  const auto &entries = reloaded.GetEntries();
  Check(entries.size() == 2, "Loaded 2 recent project entries");
  Check(entries[0].name == "Beta", "Most recent project is sorted first");
  Check(entries[1].name == "Alpha", "Older project is sorted second");
  Check(entries[0].exists && entries[1].exists,
        "Existing project files validate as present");
  Check(!entries[0].lastOpenedDisplay.empty() &&
            entries[0].lastOpenedDisplay != "Unknown",
        "LastOpenedDisplay is formatted");
}

void TestDeduplicationAndUpdate(const std::filesystem::path &sandbox) {
  const std::filesystem::path storageFile = sandbox / "recent_projects.txt";
  const std::filesystem::path alphaProj = sandbox / "Alpha" / "Alpha.elyproj";

  RecentProjectsManager manager(storageFile);
  manager.Load();
  manager.AddOrUpdateProject("Alpha Renamed", alphaProj, 1700002000);

  RecentProjectsManager reloaded(storageFile);
  reloaded.Load();
  const auto &entries = reloaded.GetEntries();
  Check(entries.size() == 2, "Updating existing project does not duplicate it");
  Check(entries[0].name == "Alpha Renamed",
        "Updated project moves to top with new name");
  Check(entries[0].lastOpenedTimestamp == 1700002000,
        "Updated project has new timestamp");
}

void TestMissingValidationAndRelocate(const std::filesystem::path &sandbox) {
  const std::filesystem::path storageFile = sandbox / "recent_projects.txt";
  const std::filesystem::path betaProj = sandbox / "Beta" / "Beta.elyproj";
  const std::filesystem::path betaMoved =
      sandbox / "BetaMoved" / "BetaMoved.elyproj";

  std::filesystem::remove(betaProj);

  RecentProjectsManager manager(storageFile);
  manager.Load();
  const auto &entries = manager.GetEntries();
  Check(entries.size() == 2, "Missing project remains listed until removed");
  Check(entries[0].exists == true, "Alpha still exists");
  Check(entries[1].exists == false, "Deleted Beta is marked missing");

  TouchFile(betaMoved);
  const bool relocated =
      manager.RelocateProject(betaProj, betaMoved, "Beta Moved");
  Check(relocated, "RelocateProject succeeds for valid target");

  RecentProjectsManager reloaded(storageFile);
  reloaded.Load();
  Check(reloaded.GetEntries()[1].name == "Beta Moved",
        "Relocated project updates name");
  Check(reloaded.GetEntries()[1].exists == true,
        "Relocated project is marked valid on disk");
}

void TestRemoveAndClearMissing(const std::filesystem::path &sandbox) {
  const std::filesystem::path storageFile = sandbox / "recent_projects.txt";
  const std::filesystem::path alphaProj = sandbox / "Alpha" / "Alpha.elyproj";
  const std::filesystem::path betaMoved =
      sandbox / "BetaMoved" / "BetaMoved.elyproj";

  std::filesystem::remove(betaMoved);

  RecentProjectsManager manager(storageFile);
  manager.Load();
  manager.ClearMissing();
  Check(manager.GetEntries().size() == 1,
        "ClearMissing removes missing entries");
  Check(manager.GetEntries()[0].name == "Alpha Renamed",
        "ClearMissing keeps existing entries");

  Check(manager.RemoveProject(alphaProj), "RemoveProject removes matching entry");
  RecentProjectsManager reloaded(storageFile);
  reloaded.Load();
  Check(reloaded.GetEntries().empty(),
        "Registry is empty after removing all entries");
}

void TestDefaultStoragePath() {
  const std::filesystem::path defaultPath =
      RecentProjectsManager::GetDefaultStoragePath();
  Check(defaultPath.filename() == "recent_projects.txt",
        "Default storage file is recent_projects.txt");
  Check(defaultPath.parent_path().filename() == "Elysium",
        "Default storage directory is Elysium");
}

int main() {
  Elysium::Log::Init();

  const std::filesystem::path sandbox = "test_sandbox_recent_projects";
  std::error_code ec;
  std::filesystem::remove_all(sandbox, ec);
  std::filesystem::create_directories(sandbox, ec);

  TestPersistenceAndSorting(sandbox);
  TestDeduplicationAndUpdate(sandbox);
  TestMissingValidationAndRelocate(sandbox);
  TestRemoveAndClearMissing(sandbox);
  TestDefaultStoragePath();

  std::filesystem::remove_all(sandbox, ec);
  std::cout << "\nAll RecentProjectsTest assertions PASSED successfully!\n";
  return 0;
}
