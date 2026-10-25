#pragma once

#include "Core.hpp"
#include "UUID.hpp"
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "Scene.hpp"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

namespace Elysium {

/**
 * @brief Handle representing an asset registered within Elysium Engine.
 */
using AssetHandle = UUID;

/**
 * @brief Asset types recognized and managed by the AssetRegistry.
 */
enum class AssetType : uint16_t {
  None = 0,
  Texture,
  Font,
  Audio,
  Scene
};

/**
 * @brief Converts an AssetType enum to a string.
 */
inline const char *AssetTypeToString(AssetType type) {
  switch (type) {
  case AssetType::Texture: return "Texture";
  case AssetType::Font:    return "Font";
  case AssetType::Audio:   return "Audio";
  case AssetType::Scene:   return "Scene";
  default:                 return "None";
  }
}

/**
 * @brief Parses an AssetType enum from a string.
 */
inline AssetType AssetTypeFromString(const std::string &typeStr) {
  if (typeStr == "Texture") return AssetType::Texture;
  if (typeStr == "Font")    return AssetType::Font;
  if (typeStr == "Audio")   return AssetType::Audio;
  if (typeStr == "Scene")   return AssetType::Scene;
  return AssetType::None;
}

/**
 * @brief Deduce AssetType from a filesystem path extension.
 */
AssetType GetAssetTypeFromPath(const std::filesystem::path &path);

/**
 * @brief Payload identifier constants for Drag-and-Drop operations.
 */
namespace AssetPayloads {
  inline constexpr const char *TEXTURE = "CONTENT_BROWSER_ITEM_TEXTURE";
  inline constexpr const char *AUDIO   = "CONTENT_BROWSER_ITEM_AUDIO";
  inline constexpr const char *FONT    = "CONTENT_BROWSER_ITEM_FONT";
  inline constexpr const char *SCENE   = "CONTENT_BROWSER_ITEM_SCENE";
  inline constexpr const char *GENERIC = "CONTENT_BROWSER_ITEM";
} // namespace AssetPayloads

/**
 * @brief Returns the appropriate drag-and-drop payload identifier for an AssetType.
 */
inline const char *GetPayloadType(AssetType type) {
  switch (type) {
  case AssetType::Texture: return AssetPayloads::TEXTURE;
  case AssetType::Audio:   return AssetPayloads::AUDIO;
  case AssetType::Font:    return AssetPayloads::FONT;
  case AssetType::Scene:   return AssetPayloads::SCENE;
  default:                 return AssetPayloads::GENERIC;
  }
}

/**
 * @brief Metadata tracking information for an asset on disk and in memory.
 */
struct AssetMetadata {
  AssetHandle handle = 0;
  AssetType type = AssetType::None;
  std::filesystem::path filePath; // Normalized project-relative path
  bool isLoaded = false;
};

/**
 * @brief Centralized Asset Manager and Registry singleton.
 *
 * Maps 64-bit UUID AssetHandles to disk paths and loaded asset memory (Textures,
 * Fonts, AudioBuffers, and Scenes). Guarantees thread-safe caching and serialization
 * across engine restarts.
 */
class ELYSIUM_API AssetManager {
public:
  static AssetManager &Get();

  // Initialization & Manifest Serialization
  void Init(const std::filesystem::path &projectAssetDirectory = "assets");
  void Shutdown();

  void SetAssetDirectory(const std::filesystem::path &dir);
  [[nodiscard]] const std::filesystem::path &GetAssetDirectory() const;

  void ReloadProjectAssets(const std::filesystem::path &projectAssetDirectory);

  bool SaveManifest(const std::filesystem::path &manifestPath = "");
  bool LoadManifest(const std::filesystem::path &manifestPath = "");

  // Registry Queries & Registration
  AssetHandle RegisterAsset(const std::filesystem::path &relativeOrFullPath,
                            AssetType type = AssetType::None,
                            AssetHandle desiredHandle = 0);
  bool UnregisterAsset(AssetHandle handle);

  bool HasAsset(AssetHandle handle) const;
  bool HasAsset(const std::filesystem::path &path) const;

  AssetHandle GetHandleFromPath(const std::filesystem::path &path) const;
  const AssetMetadata &GetMetadata(AssetHandle handle) const;
  AssetMetadata &GetMetadata(AssetHandle handle);

  const std::unordered_map<AssetHandle, AssetMetadata> &GetRegistry() const {
    return m_Registry;
  }

  // Directory Scanning / Sync
  void ScanAssetDirectory();

  // Thread-Safe Asset Loading / Caching: Load<T>(handle) & Load<T>(filepath)
  template <typename T>
  std::shared_ptr<T> Load(AssetHandle handle);

  template <typename T>
  std::shared_ptr<T> Load(const std::filesystem::path &path);

  // Explicit Cache Management
  void Unload(AssetHandle handle);
  void ClearCache();

private:
  AssetManager() = default;
  ~AssetManager() = default;
  AssetManager(const AssetManager &) = delete;
  AssetManager &operator=(const AssetManager &) = delete;

  std::filesystem::path NormalizePath(const std::filesystem::path &path) const;

private:
  mutable std::mutex m_RegistryMutex;
  mutable std::mutex m_CacheMutex;

  std::filesystem::path m_AssetDirectory = "assets";
  std::unordered_map<AssetHandle, AssetMetadata> m_Registry;
  std::unordered_map<std::string, AssetHandle> m_PathToHandle;

  // Loaded asset memory caches
  std::unordered_map<AssetHandle, std::shared_ptr<sf::Texture>> m_TextureCache;
  std::unordered_map<AssetHandle, std::shared_ptr<sf::Font>> m_FontCache;
  std::unordered_map<AssetHandle, std::shared_ptr<sf::SoundBuffer>> m_AudioCache;
  std::unordered_map<AssetHandle, std::shared_ptr<Scene>> m_SceneCache;

  static AssetMetadata s_NullMetadata;
};

// Aliases for Engine Idioms
using AssetRegistry = AssetManager;

// -----------------------------------------------------------------------------
// Template Specializations Declarations
// -----------------------------------------------------------------------------
template <>
ELYSIUM_API std::shared_ptr<sf::Texture> AssetManager::Load<sf::Texture>(AssetHandle handle);

template <>
ELYSIUM_API std::shared_ptr<sf::Font> AssetManager::Load<sf::Font>(AssetHandle handle);

template <>
ELYSIUM_API std::shared_ptr<sf::SoundBuffer> AssetManager::Load<sf::SoundBuffer>(AssetHandle handle);

template <>
ELYSIUM_API std::shared_ptr<Scene> AssetManager::Load<Scene>(AssetHandle handle);

// Generic template delegating path to handle
template <typename T>
inline std::shared_ptr<T> AssetManager::Load(const std::filesystem::path &path) {
  AssetHandle handle = RegisterAsset(path);
  return Load<T>(handle);
}

} // namespace Elysium
