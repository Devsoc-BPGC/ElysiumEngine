#include "AssetManager.hpp"
#include "Log.h"
#include "Scene.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace Elysium {

AssetMetadata AssetManager::s_NullMetadata;

AssetType GetAssetTypeFromPath(const std::filesystem::path &path) {
  std::string ext = path.extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") {
    return AssetType::Texture;
  }
  if (ext == ".ttf" || ext == ".otf") {
    return AssetType::Font;
  }
  if (ext == ".wav" || ext == ".ogg" || ext == ".flac" || ext == ".mp3") {
    return AssetType::Audio;
  }
  if (ext == ".elyscene" || ext == ".scene") {
    return AssetType::Scene;
  }
  return AssetType::None;
}

AssetManager &AssetManager::Get() {
  static AssetManager instance;
  return instance;
}

void AssetManager::Init(const std::filesystem::path &projectAssetDirectory) {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  m_AssetDirectory = projectAssetDirectory;

  if (!std::filesystem::exists(m_AssetDirectory)) {
    std::error_code ec;
    std::filesystem::create_directories(m_AssetDirectory, ec);
    if (ec) {
      ELYSIUM_CORE_WARN("AssetManager: Could not create asset directory '{}': {}",
                        m_AssetDirectory.string(), ec.message());
    }
  }

  // Attempt to load manifest if exists
  std::filesystem::path manifestPath = m_AssetDirectory / "assets.meta";
  if (std::filesystem::exists(manifestPath)) {
    m_RegistryMutex.unlock();
    LoadManifest(manifestPath);
    m_RegistryMutex.lock();
  }

  // Scan folder to pick up any untracked new files
  m_RegistryMutex.unlock();
  ScanAssetDirectory();
  m_RegistryMutex.lock();

  ELYSIUM_CORE_INFO("AssetManager initialized. Root: '{}', Total registered assets: {}",
                    m_AssetDirectory.string(), m_Registry.size());
}

void AssetManager::Shutdown() {
  SaveManifest();
  ClearCache();

  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  m_Registry.clear();
  m_PathToHandle.clear();
  ELYSIUM_CORE_INFO("AssetManager shutdown complete.");
}

void AssetManager::SetAssetDirectory(const std::filesystem::path &dir) {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  m_AssetDirectory = dir;
}

const std::filesystem::path &AssetManager::GetAssetDirectory() const {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  return m_AssetDirectory;
}

std::filesystem::path AssetManager::NormalizePath(const std::filesystem::path &path) const {
  std::filesystem::path p = path.lexically_normal();
  std::string s = p.generic_string();
  return std::filesystem::path(s);
}

bool AssetManager::SaveManifest(const std::filesystem::path &manifestPath) {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);

  std::filesystem::path actualPath = manifestPath.is_relative() 
                                     ? (m_AssetDirectory / manifestPath.filename())
                                     : manifestPath;

  std::ofstream out(actualPath);
  if (!out.is_open()) {
    ELYSIUM_CORE_ERROR("AssetManager: Failed to open manifest for writing: {}",
                       actualPath.string());
    return false;
  }

  out << "# Elysium Engine Asset Registry Manifest\n";
  out << "# Version: 1.0\n";
  out << "AssetRegistry:\n";

  for (const auto &[handle, meta] : m_Registry) {
    out << "  - Handle: " << static_cast<uint64_t>(handle) << "\n";
    out << "    Type: " << AssetTypeToString(meta.type) << "\n";
    out << "    Path: " << meta.filePath.generic_string() << "\n";
  }

  ELYSIUM_CORE_INFO("AssetManager: Manifest saved with {} entries to '{}'",
                    m_Registry.size(), actualPath.string());
  return true;
}

bool AssetManager::LoadManifest(const std::filesystem::path &manifestPath) {
  std::filesystem::path actualPath = manifestPath.is_relative()
                                     ? (m_AssetDirectory / manifestPath.filename())
                                     : manifestPath;

  if (!std::filesystem::exists(actualPath)) {
    return false;
  }

  std::ifstream in(actualPath);
  if (!in.is_open()) {
    ELYSIUM_CORE_ERROR("AssetManager: Failed to open manifest for reading: {}",
                       actualPath.string());
    return false;
  }

  std::lock_guard<std::mutex> lock(m_RegistryMutex);

  std::string line;
  AssetHandle currentHandle = 0;
  AssetType currentType = AssetType::None;
  std::string currentPathStr;

  auto commitEntry = [&]() {
    if (static_cast<uint64_t>(currentHandle) != 0 && !currentPathStr.empty()) {
      AssetMetadata meta;
      meta.handle = currentHandle;
      meta.type = currentType;
      meta.filePath = std::filesystem::path(currentPathStr).generic_string();
      meta.isLoaded = false;

      m_Registry[currentHandle] = meta;
      m_PathToHandle[meta.filePath.generic_string()] = currentHandle;
    }
    currentHandle = 0;
    currentType = AssetType::None;
    currentPathStr.clear();
  };

  while (std::getline(in, line)) {
    // Strip leading / trailing whitespace
    size_t first = line.find_first_not_of(" \t\r\n");
    if (first == std::string::npos || line[first] == '#') {
      continue;
    }
    std::string trimmed = line.substr(first);

    if (trimmed.rfind("- Handle:", 0) == 0) {
      commitEntry();
      std::string val = trimmed.substr(9);
      size_t valFirst = val.find_first_not_of(" \t");
      if (valFirst != std::string::npos) {
        currentHandle = std::stoull(val.substr(valFirst));
      }
    } else if (trimmed.rfind("Type:", 0) == 0) {
      std::string val = trimmed.substr(5);
      size_t valFirst = val.find_first_not_of(" \t");
      if (valFirst != std::string::npos) {
        currentType = AssetTypeFromString(val.substr(valFirst));
      }
    } else if (trimmed.rfind("Path:", 0) == 0) {
      std::string val = trimmed.substr(5);
      size_t valFirst = val.find_first_not_of(" \t");
      if (valFirst != std::string::npos) {
        currentPathStr = val.substr(valFirst);
      }
    }
  }
  commitEntry();

  ELYSIUM_CORE_INFO("AssetManager: Manifest loaded with {} entries from '{}'",
                    m_Registry.size(), actualPath.string());
  return true;
}

AssetHandle AssetManager::RegisterAsset(const std::filesystem::path &path,
                                        AssetType type,
                                        AssetHandle desiredHandle) {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);

  std::filesystem::path relPath = path;
  if (path.is_absolute()) {
    std::error_code ec;
    relPath = std::filesystem::relative(path, m_AssetDirectory, ec);
    if (ec) {
      relPath = path;
    }
  }
  std::string normPath = relPath.generic_string();

  // If already registered, return existing handle
  auto it = m_PathToHandle.find(normPath);
  if (it != m_PathToHandle.end()) {
    return it->second;
  }

  AssetHandle handle = (static_cast<uint64_t>(desiredHandle) != 0) ? desiredHandle : UUID();
  AssetType assetType = (type != AssetType::None) ? type : GetAssetTypeFromPath(relPath);

  AssetMetadata meta;
  meta.handle = handle;
  meta.type = assetType;
  meta.filePath = relPath;
  meta.isLoaded = false;

  m_Registry[handle] = meta;
  m_PathToHandle[normPath] = handle;

  ELYSIUM_CORE_TRACE("AssetManager: Registered [{}] '{}' as handle {}",
                     AssetTypeToString(assetType), normPath, static_cast<uint64_t>(handle));
  return handle;
}

bool AssetManager::UnregisterAsset(AssetHandle handle) {
  Unload(handle);

  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  auto it = m_Registry.find(handle);
  if (it == m_Registry.end()) {
    return false;
  }

  m_PathToHandle.erase(it->second.filePath.generic_string());
  m_Registry.erase(it);
  return true;
}

bool AssetManager::HasAsset(AssetHandle handle) const {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  return m_Registry.find(handle) != m_Registry.end();
}

bool AssetManager::HasAsset(const std::filesystem::path &path) const {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  std::string norm = path.generic_string();
  return m_PathToHandle.find(norm) != m_PathToHandle.end();
}

AssetHandle AssetManager::GetHandleFromPath(const std::filesystem::path &path) const {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  std::filesystem::path relPath = path;
  if (path.is_absolute()) {
    std::error_code ec;
    relPath = std::filesystem::relative(path, m_AssetDirectory, ec);
    if (ec) {
      relPath = path;
    }
  }
  auto it = m_PathToHandle.find(relPath.generic_string());
  if (it != m_PathToHandle.end()) {
    return it->second;
  }
  return 0;
}

const AssetMetadata &AssetManager::GetMetadata(AssetHandle handle) const {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  auto it = m_Registry.find(handle);
  if (it != m_Registry.end()) {
    return it->second;
  }
  return s_NullMetadata;
}

AssetMetadata &AssetManager::GetMetadata(AssetHandle handle) {
  std::lock_guard<std::mutex> lock(m_RegistryMutex);
  auto it = m_Registry.find(handle);
  if (it != m_Registry.end()) {
    return it->second;
  }
  return s_NullMetadata;
}

void AssetManager::ScanAssetDirectory() {
  std::filesystem::path root;
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    root = m_AssetDirectory;
  }

  if (!std::filesystem::exists(root)) {
    return;
  }

  std::error_code ec;
  for (const auto &entry : std::filesystem::recursive_directory_iterator(root, ec)) {
    if (entry.is_regular_file()) {
      std::filesystem::path p = entry.path();
      if (p.filename() == "assets.meta" || p.extension() == ".meta") {
        continue;
      }
      AssetType type = GetAssetTypeFromPath(p);
      if (type != AssetType::None) {
        RegisterAsset(p, type);
      }
    }
  }
}

void AssetManager::Unload(AssetHandle handle) {
  std::lock_guard<std::mutex> lock(m_CacheMutex);
  m_TextureCache.erase(handle);
  m_FontCache.erase(handle);
  m_AudioCache.erase(handle);
  m_SceneCache.erase(handle);

  std::lock_guard<std::mutex> regLock(m_RegistryMutex);
  auto it = m_Registry.find(handle);
  if (it != m_Registry.end()) {
    it->second.isLoaded = false;
  }
}

void AssetManager::ClearCache() {
  std::lock_guard<std::mutex> lock(m_CacheMutex);
  m_TextureCache.clear();
  m_FontCache.clear();
  m_AudioCache.clear();
  m_SceneCache.clear();

  std::lock_guard<std::mutex> regLock(m_RegistryMutex);
  for (auto &[handle, meta] : m_Registry) {
    meta.isLoaded = false;
  }
  ELYSIUM_CORE_INFO("AssetManager: Cleared all loaded memory caches.");
}

// -----------------------------------------------------------------------------
// Specializations for Load<T>(AssetHandle)
// -----------------------------------------------------------------------------

template <>
std::shared_ptr<sf::Texture> AssetManager::Load<sf::Texture>(AssetHandle handle) {
  {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    auto it = m_TextureCache.find(handle);
    if (it != m_TextureCache.end()) {
      return it->second;
    }
  }

  std::filesystem::path fullPath;
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    auto it = m_Registry.find(handle);
    if (it == m_Registry.end()) {
      ELYSIUM_CORE_WARN("AssetManager: Texture handle {} not found in registry",
                        static_cast<uint64_t>(handle));
      return nullptr;
    }
    fullPath = it->second.filePath.is_absolute()
                   ? it->second.filePath
                   : (m_AssetDirectory / it->second.filePath);
  }

  auto texture = std::make_shared<sf::Texture>();
  if (!texture->loadFromFile(fullPath)) {
    ELYSIUM_CORE_ERROR("AssetManager: Failed to load sf::Texture from '{}'",
                       fullPath.string());
    return nullptr;
  }

  {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    m_TextureCache[handle] = texture;
  }
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    m_Registry[handle].isLoaded = true;
  }
  ELYSIUM_CORE_INFO("AssetManager: Loaded sf::Texture from '{}'", fullPath.string());
  return texture;
}

template <>
std::shared_ptr<sf::Font> AssetManager::Load<sf::Font>(AssetHandle handle) {
  {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    auto it = m_FontCache.find(handle);
    if (it != m_FontCache.end()) {
      return it->second;
    }
  }

  std::filesystem::path fullPath;
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    auto it = m_Registry.find(handle);
    if (it == m_Registry.end()) {
      ELYSIUM_CORE_WARN("AssetManager: Font handle {} not found in registry",
                        static_cast<uint64_t>(handle));
      return nullptr;
    }
    fullPath = it->second.filePath.is_absolute()
                   ? it->second.filePath
                   : (m_AssetDirectory / it->second.filePath);
  }

  auto font = std::make_shared<sf::Font>();
  if (!font->openFromFile(fullPath)) {
    ELYSIUM_CORE_ERROR("AssetManager: Failed to open sf::Font from '{}'",
                       fullPath.string());
    return nullptr;
  }

  {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    m_FontCache[handle] = font;
  }
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    m_Registry[handle].isLoaded = true;
  }
  ELYSIUM_CORE_INFO("AssetManager: Loaded sf::Font from '{}'", fullPath.string());
  return font;
}

template <>
std::shared_ptr<sf::SoundBuffer> AssetManager::Load<sf::SoundBuffer>(AssetHandle handle) {
  {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    auto it = m_AudioCache.find(handle);
    if (it != m_AudioCache.end()) {
      return it->second;
    }
  }

  std::filesystem::path fullPath;
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    auto it = m_Registry.find(handle);
    if (it == m_Registry.end()) {
      ELYSIUM_CORE_WARN("AssetManager: Audio handle {} not found in registry",
                        static_cast<uint64_t>(handle));
      return nullptr;
    }
    fullPath = it->second.filePath.is_absolute()
                   ? it->second.filePath
                   : (m_AssetDirectory / it->second.filePath);
  }

  auto buffer = std::make_shared<sf::SoundBuffer>();
  if (!buffer->loadFromFile(fullPath)) {
    ELYSIUM_CORE_ERROR("AssetManager: Failed to load sf::SoundBuffer from '{}'",
                       fullPath.string());
    return nullptr;
  }

  {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    m_AudioCache[handle] = buffer;
  }
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    m_Registry[handle].isLoaded = true;
  }
  ELYSIUM_CORE_INFO("AssetManager: Loaded sf::SoundBuffer from '{}'", fullPath.string());
  return buffer;
}

template <>
std::shared_ptr<Scene> AssetManager::Load<Scene>(AssetHandle handle) {
  {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    auto it = m_SceneCache.find(handle);
    if (it != m_SceneCache.end()) {
      return it->second;
    }
  }

  std::filesystem::path fullPath;
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    auto it = m_Registry.find(handle);
    if (it == m_Registry.end()) {
      return nullptr;
    }
    fullPath = it->second.filePath.is_absolute()
                   ? it->second.filePath
                   : (m_AssetDirectory / it->second.filePath);
  }

  auto scene = std::make_shared<Scene>();
  // Placeholder scene loading logic or custom serializer
  {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    m_SceneCache[handle] = scene;
  }
  {
    std::lock_guard<std::mutex> lock(m_RegistryMutex);
    m_Registry[handle].isLoaded = true;
  }
  ELYSIUM_CORE_INFO("AssetManager: Loaded Scene from '{}'", fullPath.string());
  return scene;
}

} // namespace Elysium
