#include "panels/ContentBrowserPanel.hpp"
#include "Log.h"
#include <algorithm>
#include <cstring>
#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics/Image.hpp>

namespace Elysium {

ContentBrowserPanel::ContentBrowserPanel(const std::filesystem::path &assetDirectory)
    : EditorPanel("Content Browser"),
      m_RootDirectory(assetDirectory),
      m_CurrentDirectory(assetDirectory) {
  if (!std::filesystem::exists(m_RootDirectory)) {
    std::error_code ec;
    std::filesystem::create_directories(m_RootDirectory, ec);
  }

  // Push root to history
  m_History.push_back(m_CurrentDirectory);
  m_HistoryIndex = 0;

  InitIcons();

  std::error_code ec;
  if (std::filesystem::exists(m_RootDirectory)) {
    m_LastWriteTime = std::filesystem::last_write_time(m_RootDirectory, ec);
  }
}

void ContentBrowserPanel::InitIcons() {
  const unsigned int S = 48;

  auto createSolidIcon = [&](sf::Color bg, sf::Color fg, int style) -> std::unique_ptr<sf::Texture> {
    sf::Image img(sf::Vector2u(S, S), sf::Color(0, 0, 0, 0));

    for (unsigned int y = 0; y < S; ++y) {
      for (unsigned int x = 0; x < S; ++x) {
        if (style == 0) {
          // Folder icon: tab + body
          bool inTab = (x >= 4 && x <= 22 && y >= 6 && y <= 14);
          bool inBody = (x >= 4 && x <= 44 && y >= 14 && y <= 42);
          if (inTab || inBody) {
            bool isBorder = (x == 4 || x == 44 || y == 6 || y == 42 ||
                             (y <= 14 && (x == 22 || y == 14)));
            img.setPixel({x, y}, isBorder ? fg : bg);
          }
        } else if (style == 1) {
          // Generic document file
          bool inDoc = (x >= 8 && x <= 40 && y >= 6 && y <= 42);
          bool isFold = (x >= 28 && y <= 18 && (x + y >= 46));
          if (inDoc && !isFold) {
            bool isBorder = (x == 8 || x == 40 || y == 6 || y == 42);
            img.setPixel({x, y}, isBorder ? fg : bg);
          } else if (isFold) {
            img.setPixel({x, y}, fg);
          }
        } else if (style == 2) {
          // Texture / Image icon (Picture frame with mountain/dot)
          bool inDoc = (x >= 8 && x <= 40 && y >= 6 && y <= 42);
          if (inDoc) {
            bool isBorder = (x == 8 || x == 40 || y == 6 || y == 42);
            bool isInner = (x >= 12 && x <= 36 && y >= 10 && y <= 38);
            if (isBorder) {
              img.setPixel({x, y}, fg);
            } else if (isInner) {
              // Simple mountain triangle and sun
              bool isSun = ((x - 18) * (x - 18) + (y - 18) * (y - 18) <= 9);
              bool isMountain = (y >= 26 && ((int)x - (int)y <= 2) && ((int)y + (int)x >= 46));
              if (isSun || isMountain) {
                img.setPixel({x, y}, fg);
              } else {
                img.setPixel({x, y}, bg);
              }
            } else {
              img.setPixel({x, y}, bg);
            }
          }
        } else if (style == 3) {
          // Audio icon (Musical note)
          bool inDoc = (x >= 8 && x <= 40 && y >= 6 && y <= 42);
          if (inDoc) {
            bool isBorder = (x == 8 || x == 40 || y == 6 || y == 42);
            bool isStem = (x >= 28 && x <= 30 && y >= 14 && y <= 32);
            bool isFlag = (x >= 28 && x <= 35 && y >= 14 && y <= 18);
            bool isNoteHead = ((x >= 20 && x <= 28) && (y >= 28 && y <= 34));
            if (isBorder) {
              img.setPixel({x, y}, fg);
            } else if (isStem || isFlag || isNoteHead) {
              img.setPixel({x, y}, fg);
            } else {
              img.setPixel({x, y}, bg);
            }
          }
        } else if (style == 4) {
          // Font icon (Character 'A')
          bool inDoc = (x >= 8 && x <= 40 && y >= 6 && y <= 42);
          if (inDoc) {
            bool isBorder = (x == 8 || x == 40 || y == 6 || y == 42);
            bool isA_bar = (y == 26 && x >= 18 && x <= 30);
            bool isA_left = (x >= 16 && x <= 20 && y >= 18 && y <= 34);
            bool isA_right = (x >= 28 && x <= 32 && y >= 18 && y <= 34);
            bool isA_top = (y >= 16 && y <= 19 && x >= 18 && x <= 30);
            if (isBorder) {
              img.setPixel({x, y}, fg);
            } else if (isA_bar || isA_left || isA_right || isA_top) {
              img.setPixel({x, y}, fg);
            } else {
              img.setPixel({x, y}, bg);
            }
          }
        } else if (style == 5) {
          // Scene icon (3D box / diamond)
          bool inDoc = (x >= 8 && x <= 40 && y >= 6 && y <= 42);
          if (inDoc) {
            bool isBorder = (x == 8 || x == 40 || y == 6 || y == 42);
            int dx = std::abs((int)x - 24);
            int dy = std::abs((int)y - 24);
            bool isDiamond = (dx + dy <= 10 && dx + dy >= 8);
            if (isBorder) {
              img.setPixel({x, y}, fg);
            } else if (isDiamond) {
              img.setPixel({x, y}, fg);
            } else {
              img.setPixel({x, y}, bg);
            }
          }
        }
      }
    }

    auto tex = std::make_unique<sf::Texture>();
    (void)tex->loadFromImage(img);
    return tex;
  };

  m_FolderIcon      = createSolidIcon(sf::Color(220, 180, 80),  sf::Color(255, 230, 140), 0);
  m_FileIcon        = createSolidIcon(sf::Color(70, 75, 85),    sf::Color(180, 185, 200), 1);
  m_TextureFileIcon = createSolidIcon(sf::Color(50, 110, 160),  sf::Color(100, 200, 255), 2);
  m_AudioFileIcon   = createSolidIcon(sf::Color(140, 70, 130),  sf::Color(240, 140, 230), 3);
  m_FontFileIcon    = createSolidIcon(sf::Color(50, 130, 90),   sf::Color(120, 240, 170), 4);
  m_SceneFileIcon   = createSolidIcon(sf::Color(180, 80, 60),   sf::Color(255, 140, 100), 5);
}

const sf::Texture *ContentBrowserPanel::GetIconForPath(const std::filesystem::path &path,
                                                       bool isDir) {
  if (isDir) {
    return m_FolderIcon.get();
  }

  AssetType type = GetAssetTypeFromPath(path);
  switch (type) {
  case AssetType::Texture: return m_TextureFileIcon ? m_TextureFileIcon.get() : m_FileIcon.get();
  case AssetType::Audio:   return m_AudioFileIcon   ? m_AudioFileIcon.get()   : m_FileIcon.get();
  case AssetType::Font:    return m_FontFileIcon    ? m_FontFileIcon.get()    : m_FileIcon.get();
  case AssetType::Scene:   return m_SceneFileIcon   ? m_SceneFileIcon.get()   : m_FileIcon.get();
  default:                 return m_FileIcon.get();
  }
}

void ContentBrowserPanel::Refresh() {
  AssetManager::Get().ScanAssetDirectory();
  AssetManager::Get().SaveManifest();

  std::error_code ec;
  if (!std::filesystem::exists(m_CurrentDirectory, ec)) {
    m_CurrentDirectory = m_RootDirectory;
  }
  if (std::filesystem::exists(m_RootDirectory, ec)) {
    m_LastWriteTime = std::filesystem::last_write_time(m_RootDirectory, ec);
  }
  ELYSIUM_CORE_INFO("ContentBrowserPanel: Refreshed asset directory.");
}

void ContentBrowserPanel::CheckFileSystemChanges() {
  m_TimeSinceLastCheck += ImGui::GetIO().DeltaTime;
  if (m_TimeSinceLastCheck < 1.5f) {
    return;
  }
  m_TimeSinceLastCheck = 0.0f;

  std::error_code ec;
  if (!std::filesystem::exists(m_RootDirectory, ec)) {
    return;
  }

  auto currentWriteTime = std::filesystem::last_write_time(m_RootDirectory, ec);
  if (!ec && currentWriteTime != m_LastWriteTime) {
    m_LastWriteTime = currentWriteTime;
    Refresh();
  }
}

void ContentBrowserPanel::NavigateTo(const std::filesystem::path &dir) {
  if (dir == m_CurrentDirectory)
    return;

  // Trim future history if branching
  if (m_HistoryIndex + 1 < static_cast<int>(m_History.size())) {
    m_History.erase(m_History.begin() + m_HistoryIndex + 1, m_History.end());
  }
  m_History.push_back(dir);
  m_HistoryIndex = static_cast<int>(m_History.size()) - 1;
  m_CurrentDirectory = dir;
}

void ContentBrowserPanel::NavigateBack() {
  if (m_HistoryIndex > 0) {
    m_HistoryIndex--;
    m_CurrentDirectory = m_History[static_cast<size_t>(m_HistoryIndex)];
  }
}

void ContentBrowserPanel::NavigateForward() {
  if (m_HistoryIndex + 1 < static_cast<int>(m_History.size())) {
    m_HistoryIndex++;
    m_CurrentDirectory = m_History[static_cast<size_t>(m_HistoryIndex)];
  }
}

void ContentBrowserPanel::NavigateUp() {
  if (m_CurrentDirectory != m_RootDirectory && m_CurrentDirectory.has_parent_path()) {
    std::filesystem::path parent = m_CurrentDirectory.parent_path();
    NavigateTo(parent);
  }
}

void ContentBrowserPanel::OnImGuiRender(EditorContext &context, Scene &scene,
                                        SimpleRenderer &renderer) {
  (void)context;
  (void)scene;
  (void)renderer;

  if (!isOpen)
    return;

  CheckFileSystemChanges();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
  if (ImGui::Begin(name.c_str(), &isOpen)) {
    RenderToolbar();
    ImGui::Separator();

    // Split View: Left = Folder Tree, Right = Content Grid
    ImVec2 available = ImGui::GetContentRegionAvail();
    float treeWidth = 220.0f;

    // Left: Folder Tree panel
    ImGui::BeginChild("FolderTreeChild", ImVec2(treeWidth, available.y), true);
    RenderFolderTree();
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: Grid contents panel
    ImGui::BeginChild("ContentGridChild", ImVec2(0.0f, available.y), true);
    RenderDirectoryGrid();
    ImGui::EndChild();
  }
  ImGui::End();
  ImGui::PopStyleVar();
}

void ContentBrowserPanel::RenderToolbar() {
  bool canBack = (m_HistoryIndex > 0);
  bool canForward = (m_HistoryIndex + 1 < static_cast<int>(m_History.size()));
  bool canUp = (m_CurrentDirectory != m_RootDirectory);

  if (!canBack) ImGui::BeginDisabled();
  if (ImGui::Button("<##Back")) {
    NavigateBack();
  }
  if (!canBack) ImGui::EndDisabled();

  ImGui::SameLine();
  if (!canForward) ImGui::BeginDisabled();
  if (ImGui::Button(">##Forward")) {
    NavigateForward();
  }
  if (!canForward) ImGui::EndDisabled();

  ImGui::SameLine();
  if (!canUp) ImGui::BeginDisabled();
  if (ImGui::Button("^##Up")) {
    NavigateUp();
  }
  if (!canUp) ImGui::EndDisabled();

  ImGui::SameLine();
  if (ImGui::Button("Refresh")) {
    Refresh();
  }

  // Breadcrumb path display
  ImGui::SameLine();
  ImGui::TextDisabled("Path: ");
  ImGui::SameLine();

  std::error_code ec;
  std::filesystem::path rel = std::filesystem::relative(m_CurrentDirectory, m_RootDirectory, ec);
  std::string displayPath = ec || rel.empty() || rel == "." ? "assets" : ("assets/" + rel.generic_string());
  ImGui::TextColored(ImVec4(0.4f, 0.75f, 1.0f, 1.0f), "%s", displayPath.c_str());

  // Search Filter on right side
  ImGui::SameLine();
  float filterWidth = 180.0f;
  float avail = ImGui::GetContentRegionAvail().x;
  if (avail > filterWidth + 100.0f) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - filterWidth - 100.0f);
  }
  ImGui::SetNextItemWidth(filterWidth);
  ImGui::InputTextWithHint("##SearchFilter", "Search...", m_SearchFilter, sizeof(m_SearchFilter));

  // Thumbnail size slider
  ImGui::SameLine();
  ImGui::SetNextItemWidth(80.0f);
  ImGui::SliderFloat("##ThumbSize", &m_ThumbnailSize, 48.0f, 128.0f, "%.0f px");
}

void ContentBrowserPanel::RenderFolderTree() {
  if (ImGui::TreeNodeEx("assets", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow)) {
    if (ImGui::IsItemClicked()) {
      NavigateTo(m_RootDirectory);
    }
    RenderFolderTreeNode(m_RootDirectory);
    ImGui::TreePop();
  }
}

void ContentBrowserPanel::RenderFolderTreeNode(const std::filesystem::path &dirPath) {
  std::error_code ec;
  if (!std::filesystem::exists(dirPath, ec)) return;

  for (const auto &entry : std::filesystem::directory_iterator(dirPath, ec)) {
    if (entry.is_directory()) {
      const auto &p = entry.path();
      std::string dirName = p.filename().string();

      bool hasSubDirs = false;
      for (const auto &sub : std::filesystem::directory_iterator(p, ec)) {
        if (sub.is_directory()) {
          hasSubDirs = true;
          break;
        }
      }

      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
      if (!hasSubDirs) flags |= ImGuiTreeNodeFlags_Leaf;
      if (p == m_CurrentDirectory) flags |= ImGuiTreeNodeFlags_Selected;

      bool opened = ImGui::TreeNodeEx(dirName.c_str(), flags);
      if (ImGui::IsItemClicked()) {
        NavigateTo(p);
      }

      if (opened) {
        if (hasSubDirs) {
          RenderFolderTreeNode(p);
        }
        ImGui::TreePop();
      }
    }
  }
}

void ContentBrowserPanel::RenderDirectoryGrid() {
  std::error_code ec;
  if (!std::filesystem::exists(m_CurrentDirectory, ec)) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Directory does not exist.");
    return;
  }

  float panelWidth = ImGui::GetContentRegionAvail().x;
  float cellSize = m_ThumbnailSize + m_Padding;
  int columnCount = static_cast<int>(panelWidth / cellSize);
  if (columnCount < 1) columnCount = 1;

  if (ImGui::BeginTable("ContentGridTable", columnCount)) {
    std::string filterStr = m_SearchFilter;
    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });

    // Collect entries (directories first, then files)
    std::vector<std::filesystem::directory_entry> dirs;
    std::vector<std::filesystem::directory_entry> files;

    for (const auto &entry : std::filesystem::directory_iterator(m_CurrentDirectory, ec)) {
      if (entry.path().filename() == "assets.meta" || entry.path().extension() == ".meta") {
        continue;
      }

      std::string name = entry.path().filename().string();
      if (!filterStr.empty()) {
        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
          return static_cast<char>(std::tolower(c));
        });
        if (lowerName.find(filterStr) == std::string::npos) {
          continue;
        }
      }

      if (entry.is_directory()) {
        dirs.push_back(entry);
      } else {
        files.push_back(entry);
      }
    }

    auto sortByName = [](const auto &a, const auto &b) {
      return a.path().filename().string() < b.path().filename().string();
    };
    std::sort(dirs.begin(), dirs.end(), sortByName);
    std::sort(files.begin(), files.end(), sortByName);

    for (const auto &entry : dirs) {
      ImGui::TableNextColumn();
      RenderThumbnailItem(entry.path(), true, m_ThumbnailSize, m_Padding);
    }
    for (const auto &entry : files) {
      ImGui::TableNextColumn();
      RenderThumbnailItem(entry.path(), false, m_ThumbnailSize, m_Padding);
    }

    ImGui::EndTable();
  }
}

void ContentBrowserPanel::RenderThumbnailItem(const std::filesystem::path &itemPath,
                                             bool isDirectory,
                                             float thumbnailSize,
                                             float padding) {
  (void)padding;
  std::string filename = itemPath.filename().string();
  const sf::Texture *icon = GetIconForPath(itemPath, isDirectory);

  ImGui::PushID(filename.c_str());
  ImGui::BeginGroup();

  ImVec2 btnSize(thumbnailSize, thumbnailSize);
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.20f, 0.24f, 0.6f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.32f, 0.40f, 0.9f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.45f, 0.60f, 1.0f));

  if (icon) {
    ImGui::ImageButton(filename.c_str(), *icon, btnSize);
  } else {
    ImGui::Button("?", btnSize);
  }

  ImGui::PopStyleColor(3);

  // Drag and Drop Source
  if (!isDirectory && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
    AssetType type = GetAssetTypeFromPath(itemPath);
    AssetHandle handle = AssetManager::Get().RegisterAsset(itemPath, type);

    const char *payloadType = GetPayloadType(type);
    uint64_t rawHandle = static_cast<uint64_t>(handle);

    // Payload contains the 64-bit raw AssetHandle (UUID)
    ImGui::SetDragDropPayload(payloadType, &rawHandle, sizeof(uint64_t));
    if (std::strcmp(payloadType, AssetPayloads::GENERIC) != 0) {
      ImGui::SetDragDropPayload(AssetPayloads::GENERIC, &rawHandle, sizeof(uint64_t));
    }

    // Drag-and-drop preview
    ImGui::Text("Asset: %s", filename.c_str());
    ImGui::TextDisabled("Type: %s (ID: %llu)", AssetTypeToString(type),
                        static_cast<unsigned long long>(rawHandle));
    ImGui::EndDragDropSource();
  }

  // Navigation or Activation on Double Click
  if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    if (isDirectory) {
      NavigateTo(itemPath);
    } else {
      AssetType type = GetAssetTypeFromPath(itemPath);
      AssetHandle handle = AssetManager::Get().RegisterAsset(itemPath, type);
      ELYSIUM_INFO("ContentBrowser: Activated item '{}' (Handle: {})",
                   filename, static_cast<uint64_t>(handle));
    }
  }

  // Truncated / wrapped text label below icon
  float wrapWidth = thumbnailSize + 12.0f;
  std::string labelText = filename;
  if (labelText.length() > 14) {
    labelText = labelText.substr(0, 11) + "...";
  }
  float textWidth = ImGui::CalcTextSize(labelText.c_str()).x;
  float offset = (wrapWidth - textWidth) * 0.5f;
  if (offset > 0.0f) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
  }
  ImGui::TextUnformatted(labelText.c_str());

  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", filename.c_str());
  }

  ImGui::EndGroup();
  ImGui::PopID();
}

} // namespace Elysium
