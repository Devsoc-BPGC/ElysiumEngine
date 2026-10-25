#pragma once

#include "AssetManager.hpp"
#include "EditorPanel.hpp"
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <SFML/Graphics/Texture.hpp>

namespace Elysium {

/**
 * @brief Interactive Content Browser Editor Panel for exploring, managing, and
 * dragging assets into the Elysium Engine scene.
 */
class ContentBrowserPanel : public EditorPanel {
public:
  explicit ContentBrowserPanel(const std::filesystem::path &assetDirectory = "");
  ~ContentBrowserPanel() override = default;

  void OnImGuiRender(EditorContext &context, Scene &scene,
                     SimpleRenderer &renderer) override;

  void Refresh();
  void SetRootDirectory(const std::filesystem::path &dir);

private:
  void RenderToolbar();
  void RenderFolderTree();
  void RenderFolderTreeNode(const std::filesystem::path &dirPath);
  void RenderDirectoryGrid();
  void RenderThumbnailItem(const std::filesystem::path &itemPath,
                           bool isDirectory,
                           float thumbnailSize,
                           float padding);

  void NavigateTo(const std::filesystem::path &dir);
  void NavigateBack();
  void NavigateForward();
  void NavigateUp();

  void InitIcons();
  const sf::Texture *GetIconForPath(const std::filesystem::path &path, bool isDir);

  void CheckFileSystemChanges();

private:
  std::filesystem::path m_RootDirectory;
  std::filesystem::path m_CurrentDirectory;

  // Navigation History
  std::vector<std::filesystem::path> m_History;
  int m_HistoryIndex = -1;

  // Search Filter
  char m_SearchFilter[128] = {0};

  // Thumbnail rendering parameters
  float m_ThumbnailSize = 80.0f;
  float m_Padding = 12.0f;

  // File system watcher tracking
  std::chrono::file_clock::time_point m_LastWriteTime;
  float m_TimeSinceLastCheck = 0.0f;

  // Visual Icons / Thumbnails
  std::unique_ptr<sf::Texture> m_FolderIcon;
  std::unique_ptr<sf::Texture> m_FileIcon;
  std::unique_ptr<sf::Texture> m_TextureFileIcon;
  std::unique_ptr<sf::Texture> m_AudioFileIcon;
  std::unique_ptr<sf::Texture> m_FontFileIcon;
  std::unique_ptr<sf::Texture> m_SceneFileIcon;
};

} // namespace Elysium
