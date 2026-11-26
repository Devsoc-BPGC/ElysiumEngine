#include "elypch.hpp"
#include "RecentProjects.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>

namespace Elysium {

namespace {

std::string TrimWhitespace(const std::string &str) {
  const std::size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }
  const std::size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, last - first + 1);
}

std::int64_t CurrentUnixTimestamp() {
  const auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(
             now.time_since_epoch())
      .count();
}

} // namespace

RecentProjectsManager::RecentProjectsManager(std::filesystem::path storagePath)
    : m_StoragePath(storagePath.empty() ? GetDefaultStoragePath()
                                        : std::move(storagePath)) {}

void RecentProjectsManager::SetStoragePath(std::filesystem::path storagePath) {
  m_StoragePath = storagePath.empty() ? GetDefaultStoragePath()
                                      : std::move(storagePath);
}

std::filesystem::path RecentProjectsManager::GetAppDataDirectory() {
#if defined(ELYSIUM_PLATFORM_WINDOWS) || defined(_WIN32)
  const char *appData = std::getenv("APPDATA");
  if (appData && appData[0] != '\0') {
    return std::filesystem::path(appData) / "Elysium";
  }
  const char *userProfile = std::getenv("USERPROFILE");
  if (userProfile && userProfile[0] != '\0') {
    return std::filesystem::path(userProfile) / "AppData" / "Roaming" /
           "Elysium";
  }
  return std::filesystem::current_path() / "Elysium";
#elif defined(ELYSIUM_PLATFORM_MAC) || defined(__APPLE__)
  const char *home = std::getenv("HOME");
  if (home && home[0] != '\0') {
    return std::filesystem::path(home) / "Library" / "Application Support" /
           "Elysium";
  }
  return std::filesystem::current_path() / "Elysium";
#else
  const char *xdgConfig = std::getenv("XDG_CONFIG_HOME");
  if (xdgConfig && xdgConfig[0] != '\0') {
    return std::filesystem::path(xdgConfig) / "Elysium";
  }
  const char *home = std::getenv("HOME");
  if (home && home[0] != '\0') {
    return std::filesystem::path(home) / ".config" / "Elysium";
  }
  return std::filesystem::current_path() / ".config" / "Elysium";
#endif
}

std::filesystem::path RecentProjectsManager::GetDefaultStoragePath() {
  return GetAppDataDirectory() / "recent_projects.txt";
}

std::string RecentProjectsManager::FormatTimestamp(std::int64_t epochSeconds) {
  if (epochSeconds <= 0) {
    return "Unknown";
  }

  const std::time_t rawTime = static_cast<std::time_t>(epochSeconds);
  std::tm localTm{};
#if defined(ELYSIUM_PLATFORM_WINDOWS) || defined(_WIN32)
  if (localtime_s(&localTm, &rawTime) != 0) {
    return "Unknown";
  }
#else
  if (localtime_r(&rawTime, &localTm) == nullptr) {
    return "Unknown";
  }
#endif

  char buffer[32] = {};
  if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &localTm) == 0) {
    return "Unknown";
  }
  return std::string(buffer);
}

std::filesystem::path
RecentProjectsManager::NormalizeProjectPath(const std::filesystem::path &path) {
  if (path.empty()) {
    return {};
  }

  std::error_code ec;
  std::filesystem::path absPath = std::filesystem::absolute(path, ec);
  if (ec) {
    return path.lexically_normal();
  }

  if (std::filesystem::exists(absPath, ec)) {
    std::filesystem::path canonicalPath =
        std::filesystem::canonical(absPath, ec);
    if (!ec) {
      return canonicalPath;
    }
  }
  return absPath.lexically_normal();
}

bool RecentProjectsManager::PathsMatch(const std::filesystem::path &a,
                                       const std::filesystem::path &b) {
  if (a.empty() || b.empty()) {
    return false;
  }
  const std::filesystem::path normA = NormalizeProjectPath(a);
  const std::filesystem::path normB = NormalizeProjectPath(b);
  if (normA.generic_string() == normB.generic_string()) {
    return true;
  }

  std::error_code ec;
  if (std::filesystem::exists(normA, ec) &&
      std::filesystem::exists(normB, ec)) {
    return std::filesystem::equivalent(normA, normB, ec);
  }
  return false;
}

void RecentProjectsManager::Load() {
  m_Entries.clear();

  std::error_code ec;
  if (!std::filesystem::exists(m_StoragePath, ec)) {
    return;
  }

  std::ifstream in(m_StoragePath);
  if (!in.is_open()) {
    ELYSIUM_CORE_WARN("RecentProjectsManager: Failed to open '{}'",
                      m_StoragePath.string());
    return;
  }

  RecentProjectEntry current;
  bool inBlock = false;

  auto commitBlock = [&]() {
    if (!inBlock || current.projectFilePath.empty()) {
      current = RecentProjectEntry{};
      inBlock = false;
      return;
    }

    if (current.name.empty()) {
      current.name = current.projectFilePath.stem().string();
      if (current.name.empty()) {
        current.name = "Untitled";
      }
    }
    current.lastOpenedDisplay = FormatTimestamp(current.lastOpenedTimestamp);

    // Deduplicate while loading
    const bool duplicate = std::any_of(
        m_Entries.begin(), m_Entries.end(), [&](const RecentProjectEntry &e) {
          return PathsMatch(e.projectFilePath, current.projectFilePath);
        });
    if (!duplicate) {
      m_Entries.push_back(std::move(current));
    }

    current = RecentProjectEntry{};
    inBlock = false;
  };

  std::string line;
  while (std::getline(in, line)) {
    const std::string trimmed = TrimWhitespace(line);
    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }

    if (trimmed == "[Project]") {
      commitBlock();
      inBlock = true;
      continue;
    }

    if (!inBlock) {
      continue;
    }

    const std::size_t eqPos = trimmed.find('=');
    if (eqPos == std::string::npos) {
      continue;
    }

    const std::string key = TrimWhitespace(trimmed.substr(0, eqPos));
    const std::string val = TrimWhitespace(trimmed.substr(eqPos + 1));

    if (key == "Name") {
      current.name = val;
    } else if (key == "Path") {
      current.projectFilePath = std::filesystem::path(val).lexically_normal();
    } else if (key == "LastOpened") {
      try {
        current.lastOpenedTimestamp = std::stoll(val);
      } catch (...) {
        current.lastOpenedTimestamp = 0;
      }
    }
  }
  commitBlock();

  ValidateEntries();
  SortAndTrim();
}

bool RecentProjectsManager::Save() const {
  std::error_code ec;
  const std::filesystem::path parentDir = m_StoragePath.parent_path();
  if (!parentDir.empty() && !std::filesystem::exists(parentDir, ec)) {
    std::filesystem::create_directories(parentDir, ec);
    if (ec) {
      ELYSIUM_CORE_ERROR(
          "RecentProjectsManager: Failed to create directory '{}': {}",
          parentDir.string(), ec.message());
      return false;
    }
  }

  std::ofstream out(m_StoragePath);
  if (!out.is_open()) {
    ELYSIUM_CORE_ERROR("RecentProjectsManager: Failed to write '{}'",
                       m_StoragePath.string());
    return false;
  }

  out << "# Elysium Engine Recent Projects Registry\n";
  out << "# Version: 1.0\n\n";

  for (const auto &entry : m_Entries) {
    out << "[Project]\n";
    out << "Name=" << entry.name << "\n";
    out << "Path=" << entry.projectFilePath.generic_string() << "\n";
    out << "LastOpened=" << entry.lastOpenedTimestamp << "\n\n";
  }

  return out.good();
}

void RecentProjectsManager::ValidateEntries() {
  std::error_code ec;
  for (auto &entry : m_Entries) {
    entry.exists =
        !entry.projectFilePath.empty() &&
        std::filesystem::exists(entry.projectFilePath, ec) &&
        std::filesystem::is_regular_file(entry.projectFilePath, ec);
    entry.lastOpenedDisplay = FormatTimestamp(entry.lastOpenedTimestamp);
  }
}

void RecentProjectsManager::SortAndTrim() {
  std::stable_sort(
      m_Entries.begin(), m_Entries.end(),
      [](const RecentProjectEntry &a, const RecentProjectEntry &b) {
        return a.lastOpenedTimestamp > b.lastOpenedTimestamp;
      });
  if (m_Entries.size() > kMaxRecentProjects) {
    m_Entries.resize(kMaxRecentProjects);
  }
}

void RecentProjectsManager::AddOrUpdateProject(
    const std::string &name, const std::filesystem::path &projectFilePath,
    std::int64_t timestamp) {
  if (projectFilePath.empty()) {
    return;
  }

  const std::filesystem::path normalized = NormalizeProjectPath(projectFilePath);
  const std::int64_t effectiveTimestamp =
      (timestamp > 0) ? timestamp : CurrentUnixTimestamp();
  const std::string effectiveName =
      name.empty() ? normalized.stem().string() : name;

  std::error_code ec;
  const bool fileExists = std::filesystem::exists(normalized, ec) &&
                          std::filesystem::is_regular_file(normalized, ec);

  for (auto &entry : m_Entries) {
    if (PathsMatch(entry.projectFilePath, normalized)) {
      entry.name = effectiveName;
      entry.projectFilePath = normalized;
      entry.lastOpenedTimestamp = effectiveTimestamp;
      entry.lastOpenedDisplay = FormatTimestamp(effectiveTimestamp);
      entry.exists = fileExists;
      SortAndTrim();
      Save();
      return;
    }
  }

  RecentProjectEntry newEntry;
  newEntry.name = effectiveName;
  newEntry.projectFilePath = normalized;
  newEntry.lastOpenedTimestamp = effectiveTimestamp;
  newEntry.lastOpenedDisplay = FormatTimestamp(effectiveTimestamp);
  newEntry.exists = fileExists;

  m_Entries.push_back(std::move(newEntry));
  SortAndTrim();
  Save();
}

bool RecentProjectsManager::RemoveProject(
    const std::filesystem::path &projectFilePath) {
  const auto oldSize = m_Entries.size();
  m_Entries.erase(
      std::remove_if(m_Entries.begin(), m_Entries.end(),
                     [&](const RecentProjectEntry &entry) {
                       return PathsMatch(entry.projectFilePath,
                                         projectFilePath);
                     }),
      m_Entries.end());

  if (m_Entries.size() != oldSize) {
    Save();
    return true;
  }
  return false;
}

bool RecentProjectsManager::RelocateProject(
    const std::filesystem::path &oldProjectFilePath,
    const std::filesystem::path &newProjectFilePath,
    const std::string &newName) {
  if (newProjectFilePath.empty()) {
    return false;
  }

  const std::filesystem::path normalizedNew =
      NormalizeProjectPath(newProjectFilePath);

  std::error_code ec;
  const bool newExists = std::filesystem::exists(normalizedNew, ec) &&
                         std::filesystem::is_regular_file(normalizedNew, ec);
  if (!newExists) {
    return false;
  }

  auto targetIt = std::find_if(
      m_Entries.begin(), m_Entries.end(),
      [&](const RecentProjectEntry &entry) {
        return PathsMatch(entry.projectFilePath, oldProjectFilePath);
      });
  if (targetIt == m_Entries.end()) {
    return false;
  }

  const std::string resolvedName =
      !newName.empty()
          ? newName
          : (targetIt->name.empty() ? normalizedNew.stem().string()
                                    : targetIt->name);
  const std::int64_t preservedTimestamp = targetIt->lastOpenedTimestamp;

  // Remove any pre-existing duplicate entry pointing to normalizedNew
  for (auto it = m_Entries.begin(); it != m_Entries.end();) {
    if (it != targetIt && PathsMatch(it->projectFilePath, normalizedNew)) {
      const bool targetAfter = targetIt > it;
      it = m_Entries.erase(it);
      if (targetAfter) {
        --targetIt;
      }
    } else {
      ++it;
    }
  }

  targetIt->name = resolvedName;
  targetIt->projectFilePath = normalizedNew;
  targetIt->lastOpenedTimestamp =
      (preservedTimestamp > 0) ? preservedTimestamp : CurrentUnixTimestamp();
  targetIt->lastOpenedDisplay = FormatTimestamp(targetIt->lastOpenedTimestamp);
  targetIt->exists = true;

  SortAndTrim();
  Save();
  return true;
}

void RecentProjectsManager::ClearMissing() {
  ValidateEntries();
  const auto oldSize = m_Entries.size();
  m_Entries.erase(std::remove_if(m_Entries.begin(), m_Entries.end(),
                                 [](const RecentProjectEntry &entry) {
                                   return !entry.exists;
                                 }),
                  m_Entries.end());
  if (m_Entries.size() != oldSize) {
    Save();
  }
}

} // namespace Elysium
