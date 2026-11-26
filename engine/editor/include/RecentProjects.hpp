#pragma once

#include "Core.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Elysium {

/**
 * @brief Metadata for a single recently opened or created Elysium project.
 */
struct RecentProjectEntry {
  std::string name;
  std::filesystem::path projectFilePath;
  std::int64_t lastOpenedTimestamp = 0; // Unix epoch seconds
  std::string lastOpenedDisplay;
  bool exists = true;
};

/**
 * @brief Persistent registry of recently opened Elysium projects.
 *
 * Stores project entries in the user's platform-standard application data
 * directory (~/.config/Elysium/recent_projects.txt on Linux,
 * %APPDATA%/Elysium/recent_projects.txt on Windows, and
 * ~/Library/Application Support/Elysium/recent_projects.txt on macOS).
 */
class ELYSIUM_API RecentProjectsManager {
public:
  static constexpr std::size_t kMaxRecentProjects = 25;

  explicit RecentProjectsManager(std::filesystem::path storagePath = {});

  /**
   * @brief Loads recent projects from disk, validates existence, and sorts
   *        most-recently-opened first.
   */
  void Load();

  /**
   * @brief Persists the current recent projects list to disk.
   */
  bool Save() const;

  /**
   * @brief Re-checks whether each entry's .elyproj file exists on disk.
   */
  void ValidateEntries();

  /**
   * @brief Adds a project or updates its name and timestamp if already listed.
   * @param name Display name of the project.
   * @param projectFilePath Path to the .elyproj file.
   * @param timestamp Optional Unix epoch timestamp; 0 uses current time.
   */
  void AddOrUpdateProject(const std::string &name,
                          const std::filesystem::path &projectFilePath,
                          std::int64_t timestamp = 0);

  /**
   * @brief Removes a project entry matching the given .elyproj path.
   */
  bool RemoveProject(const std::filesystem::path &projectFilePath);

  /**
   * @brief Updates a missing (or moved) project's path to a new .elyproj location.
   */
  bool RelocateProject(const std::filesystem::path &oldProjectFilePath,
                       const std::filesystem::path &newProjectFilePath,
                       const std::string &newName = "");

  /**
   * @brief Removes all entries whose .elyproj file does not exist on disk.
   */
  void ClearMissing();

  [[nodiscard]] const std::vector<RecentProjectEntry> &GetEntries() const {
    return m_Entries;
  }

  [[nodiscard]] const std::filesystem::path &GetStoragePath() const {
    return m_StoragePath;
  }

  void SetStoragePath(std::filesystem::path storagePath);

  [[nodiscard]] static std::filesystem::path GetAppDataDirectory();
  [[nodiscard]] static std::filesystem::path GetDefaultStoragePath();
  [[nodiscard]] static std::string FormatTimestamp(std::int64_t epochSeconds);

private:
  [[nodiscard]] static std::filesystem::path
  NormalizeProjectPath(const std::filesystem::path &path);
  [[nodiscard]] static bool PathsMatch(const std::filesystem::path &a,
                                       const std::filesystem::path &b);
  void SortAndTrim();

  std::filesystem::path m_StoragePath;
  std::vector<RecentProjectEntry> m_Entries;
};

} // namespace Elysium
