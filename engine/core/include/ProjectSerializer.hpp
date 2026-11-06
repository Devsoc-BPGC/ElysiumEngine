#pragma once

#include "Core.hpp"
#include "Project.hpp"
#include <filesystem>
#include <memory>

namespace Elysium {

/**
 * @brief Serializer and deserializer for Elysium project files (.elyproj).
 */
class ELYSIUM_API ProjectSerializer {
public:
  explicit ProjectSerializer(std::shared_ptr<Project> project)
      : m_Project(std::move(project)) {}

  bool Serialize(const std::filesystem::path &filepath);
  bool Deserialize(const std::filesystem::path &filepath);

private:
  std::shared_ptr<Project> m_Project;
};

} // namespace Elysium
