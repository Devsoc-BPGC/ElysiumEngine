#include "ProjectSerializer.hpp"
#include "Log.h"
#include <fstream>
#include <sstream>

namespace Elysium {

bool ProjectSerializer::Serialize(const std::filesystem::path &filepath) {
  if (!m_Project) {
    ELYSIUM_CORE_ERROR("ProjectSerializer::Serialize: No project instance attached!");
    return false;
  }

  const auto &config = m_Project->GetConfig();

  std::ofstream out(filepath);
  if (!out.is_open()) {
    ELYSIUM_CORE_ERROR("ProjectSerializer: Failed to open project file for writing: {}",
                       filepath.string());
    return false;
  }

  out << "# Elysium Engine Project File\n";
  out << "# Version: 1.0\n";
  out << "Project:\n";
  out << "  Name: " << config.Name << "\n";
  out << "  AssetDirectory: " << config.AssetDirectory.generic_string() << "\n";
  out << "  StartScene: " << config.StartScenePath.generic_string() << "\n";

  ELYSIUM_CORE_INFO("ProjectSerializer: Serialized project '{}' to '{}'",
                    config.Name, filepath.string());
  return true;
}

bool ProjectSerializer::Deserialize(const std::filesystem::path &filepath) {
  if (!std::filesystem::exists(filepath)) {
    ELYSIUM_CORE_ERROR("ProjectSerializer: File not found: {}", filepath.string());
    return false;
  }

  std::ifstream in(filepath);
  if (!in.is_open()) {
    ELYSIUM_CORE_ERROR("ProjectSerializer: Failed to open project file: {}",
                       filepath.string());
    return false;
  }

  if (!m_Project) {
    m_Project = std::make_shared<Project>();
  }

  std::error_code ec;
  std::filesystem::path parentDir = filepath.parent_path();
  std::filesystem::path canonicalParent = std::filesystem::canonical(parentDir, ec);
  if (ec) {
    canonicalParent = std::filesystem::absolute(parentDir).lexically_normal();
  }

  auto &config = m_Project->GetConfig();
  config.ProjectDirectory = canonicalParent;

  std::string line;
  while (std::getline(in, line)) {
    size_t first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || line[first] == '#') {
      continue;
    }
    std::string trimmed = line.substr(first);

    if (trimmed.rfind("Name:", 0) == 0) {
      std::string val = trimmed.substr(5);
      size_t valFirst = val.find_first_not_of(" \t");
      if (valFirst != std::string::npos) {
        config.Name = val.substr(valFirst);
      }
    } else if (trimmed.rfind("AssetDirectory:", 0) == 0) {
      std::string val = trimmed.substr(15);
      size_t valFirst = val.find_first_not_of(" \t");
      if (valFirst != std::string::npos) {
        config.AssetDirectory = val.substr(valFirst);
      }
    } else if (trimmed.rfind("StartScene:", 0) == 0) {
      std::string val = trimmed.substr(11);
      size_t valFirst = val.find_first_not_of(" \t");
      if (valFirst != std::string::npos) {
        config.StartScenePath = val.substr(valFirst);
      }
    }
  }

  // Fallback defaults if empty
  if (config.Name.empty()) {
    config.Name = filepath.stem().string();
  }
  if (config.AssetDirectory.empty()) {
    config.AssetDirectory = "Assets";
  }

  ELYSIUM_CORE_INFO("ProjectSerializer: Deserialized project '{}' from '{}'",
                    config.Name, filepath.string());
  return true;
}

} // namespace Elysium
