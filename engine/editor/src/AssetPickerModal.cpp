#include "elypch.hpp"
#include "AssetPickerModal.hpp"
#include "FileDialog.hpp"
#include "Project.hpp"
#include <imgui.h>
#include <imgui-SFML.h>
#include <algorithm>
#include <cctype>

namespace Elysium {

bool AssetPickerModal::s_IsOpen = false;
bool AssetPickerModal::s_OpenPopupRequested = false;
bool AssetPickerModal::s_JustOpened = false;
std::string AssetPickerModal::s_Title = "Select Asset";
AssetType AssetPickerModal::s_TargetType = AssetType::None;
AssetHandle AssetPickerModal::s_CurrentSelection = 0;
AssetHandle AssetPickerModal::s_TempSelectedHandle = 0;
std::filesystem::path AssetPickerModal::s_TempSelectedPath = "";
AssetPickerModal::AssetSelectedCallback AssetPickerModal::s_Callback = nullptr;
char AssetPickerModal::s_SearchBuffer[256] = "";
bool AssetPickerModal::s_IsGridView = true;
std::vector<AssetItem> AssetPickerModal::s_CachedItems;

void AssetPickerModal::Open(const std::string &title, AssetType type,
                            AssetHandle currentSelection,
                            AssetSelectedCallback onSelect) {
  s_IsOpen = true;
  s_OpenPopupRequested = true;
  s_JustOpened = true;
  s_Title = title.empty() ? ("Select " + std::string(AssetTypeToString(type))) : title;
  s_TargetType = type;
  s_CurrentSelection = currentSelection;
  s_TempSelectedHandle = currentSelection;
  s_Callback = std::move(onSelect);
  s_SearchBuffer[0] = '\0';
  s_IsGridView = (type == AssetType::Texture);

  if (static_cast<uint64_t>(currentSelection) != 0) {
    const auto &meta = AssetManager::Get().GetMetadata(currentSelection);
    s_TempSelectedPath = (static_cast<uint64_t>(meta.handle) != 0) ? meta.filePath : "";
  } else {
    s_TempSelectedPath.clear();
  }

  RefreshItems();
}

bool AssetPickerModal::IsOpen() {
  return s_IsOpen;
}

void AssetPickerModal::Close() {
  s_IsOpen = false;
  s_OpenPopupRequested = false;
  s_Callback = nullptr;
}

void AssetPickerModal::RefreshItems() {
  AssetManager::Get().ScanAssetDirectory();
  s_CachedItems = AssetManager::Get().GetAssetsByType(s_TargetType);
}

bool AssetPickerModal::FilterMatches(const AssetItem &item) {
  if (s_SearchBuffer[0] == '\0') {
    return true;
  }

  std::string query = s_SearchBuffer;
  std::transform(query.begin(), query.end(), query.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  std::string filenameLower = item.filename;
  std::transform(filenameLower.begin(), filenameLower.end(), filenameLower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  if (filenameLower.find(query) != std::string::npos) {
    return true;
  }

  std::string pathLower = item.relativePath.generic_string();
  std::transform(pathLower.begin(), pathLower.end(), pathLower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  return (pathLower.find(query) != std::string::npos);
}

void AssetPickerModal::ConfirmSelection(AssetHandle handle,
                                        const std::filesystem::path &path) {
  auto cb = std::move(s_Callback);
  Close();
  if (cb) {
    cb(handle, path);
  }
}

void AssetPickerModal::ImportFromDisk() {
  std::vector<FileDialogFilter> filters;
  switch (s_TargetType) {
  case AssetType::Texture:
    filters = {{"Image Files (*.png, *.jpg, *.jpeg, *.tga, *.bmp)",
                "*.png *.jpg *.jpeg *.tga *.bmp"},
               {"All Files", "*"}};
    break;
  case AssetType::Audio:
    filters = {{"Audio Files (*.wav, *.ogg, *.mp3, *.flac)",
                "*.wav *.ogg *.mp3 *.flac"},
               {"All Files", "*"}};
    break;
  case AssetType::Font:
    filters = {{"Font Files (*.ttf, *.otf)", "*.ttf *.otf"},
               {"All Files", "*"}};
    break;
  case AssetType::Scene:
    filters = {{"Scene Files (*.elyscene, *.scene)", "*.elyscene *.scene"},
               {"All Files", "*"}};
    break;
  default:
    filters = {{"All Files", "*"}};
    break;
  }

  std::filesystem::path assetDir = Project::GetActiveAssetDirectory();
  if (assetDir.empty()) {
    assetDir = AssetManager::Get().GetAssetDirectory();
  }

  auto selected = FileDialog::OpenFile("Import Asset", assetDir, filters);
  if (selected.has_value() && !selected->empty()) {
    std::filesystem::path srcPath = *selected;
    std::error_code ec;

    std::filesystem::path relPath;
    std::filesystem::path relCheck = std::filesystem::relative(srcPath, assetDir, ec);
    if (!ec && !relCheck.empty() && relCheck.string().rfind("..", 0) != 0) {
      relPath = relCheck;
    } else {
      if (!std::filesystem::exists(assetDir, ec)) {
        std::filesystem::create_directories(assetDir, ec);
      }
      std::filesystem::path destPath = assetDir / srcPath.filename();
      std::filesystem::copy_file(srcPath, destPath,
                                 std::filesystem::copy_options::overwrite_existing, ec);
      relPath = srcPath.filename();
    }

    AssetHandle handle = AssetManager::Get().RegisterAsset(relPath, s_TargetType);
    RefreshItems();
    ConfirmSelection(handle, relPath);
  }
}

static void DrawTruncatedFilename(const std::string &text, float maxWidth) {
  if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) {
    float textWidth = ImGui::CalcTextSize(text.c_str()).x;
    if (maxWidth > textWidth) {
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (maxWidth - textWidth) * 0.5f);
    }
    ImGui::TextUnformatted(text.c_str());
    return;
  }

  std::string truncated = text;
  while (truncated.size() > 3 &&
         ImGui::CalcTextSize((truncated + "...").c_str()).x > maxWidth) {
    truncated.pop_back();
  }
  truncated += "...";
  float textWidth = ImGui::CalcTextSize(truncated.c_str()).x;
  if (maxWidth > textWidth) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (maxWidth - textWidth) * 0.5f);
  }
  ImGui::TextUnformatted(truncated.c_str());
}

void AssetPickerModal::Draw() {
  if (!s_IsOpen && !s_OpenPopupRequested) {
    return;
  }

  if (s_OpenPopupRequested) {
    ImGui::OpenPopup(s_Title.c_str());
    s_OpenPopupRequested = false;
  }

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
  ImGui::SetNextWindowSize(ImVec2(660.0f, 500.0f), ImGuiCond_Appearing);

  bool p_open = true;
  if (ImGui::BeginPopupModal(s_Title.c_str(), &p_open,
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse)) {
    if (!p_open || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
      Close();
      ImGui::CloseCurrentPopup();
      ImGui::EndPopup();
      return;
    }

    // Top Header / Search Row
    if (s_JustOpened) {
      ImGui::SetKeyboardFocusHere();
      s_JustOpened = false;
    }

    float availHeaderWidth = ImGui::GetContentRegionAvail().x;
    float searchWidth = availHeaderWidth - 145.0f;
    if (searchWidth < 120.0f) searchWidth = 120.0f;

    ImGui::SetNextItemWidth(searchWidth);
    ImGui::InputTextWithHint("##AssetSearch", "Search assets...", s_SearchBuffer,
                             sizeof(s_SearchBuffer));
    ImGui::SameLine();
    if (ImGui::Button("X##ClearSearch", ImVec2(24.0f, 0.0f))) {
      s_SearchBuffer[0] = '\0';
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Clear search text");
    }

    ImGui::SameLine();
    if (ImGui::Button(s_IsGridView ? "List View" : "Grid View", ImVec2(100.0f, 0.0f))) {
      s_IsGridView = !s_IsGridView;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Scrollable Content Area
    float footerHeight = ImGui::GetFrameHeightWithSpacing() + 8.0f;
    ImGui::BeginChild("AssetPickerContent", ImVec2(0, -footerHeight), true);

    bool requestConfirm = false;

    if (s_IsGridView) {
      const float thumbnailSize = 64.0f;
      const float cellPadding = 12.0f;
      const float cellWidth = thumbnailSize + cellPadding;
      float contentWidth = ImGui::GetContentRegionAvail().x;
      int columnCount = static_cast<int>(contentWidth / cellWidth);
      if (columnCount < 1) columnCount = 1;

      if (ImGui::BeginTable("AssetGridTable", columnCount)) {
        // Tile 0: "None (Clear)" entry
        ImGui::TableNextColumn();
        ImGui::PushID("AssetTile_None");
        ImGui::BeginGroup();

        bool isNoneSelected = (static_cast<uint64_t>(s_TempSelectedHandle) == 0);
        if (isNoneSelected) {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.70f, 0.8f));
          ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 0.70f, 1.0f, 1.0f));
          ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
        } else {
          ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.18f, 0.22f, 0.7f));
        }

        if (ImGui::Button("[None]\nClear", ImVec2(thumbnailSize, thumbnailSize))) {
          s_TempSelectedHandle = 0;
          s_TempSelectedPath.clear();
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          s_TempSelectedHandle = 0;
          s_TempSelectedPath.clear();
          requestConfirm = true;
        }

        if (isNoneSelected) {
          ImGui::PopStyleVar();
          ImGui::PopStyleColor(2);
        } else {
          ImGui::PopStyleColor(1);
        }

        DrawTruncatedFilename("None (Clear)", thumbnailSize);

        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Clear current assignment");
        }

        ImGui::EndGroup();
        ImGui::PopID();

        // Render matching asset tiles
        for (const auto &item : s_CachedItems) {
          if (!FilterMatches(item)) {
            continue;
          }

          ImGui::TableNextColumn();
          ImGui::PushID(item.relativePath.generic_string().c_str());
          ImGui::BeginGroup();

          bool isSelected = (s_TempSelectedHandle == item.handle);
          if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.70f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.40f, 0.70f, 1.0f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
          } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.20f, 0.24f, 0.6f));
          }

          std::shared_ptr<sf::Texture> thumbTex = nullptr;
          if (item.type == AssetType::Texture) {
            thumbTex = AssetManager::Get().Load<sf::Texture>(item.handle);
          }

          if (thumbTex) {
            if (ImGui::ImageButton("##ThumbBtn", *thumbTex,
                                   sf::Vector2f(thumbnailSize - 8.0f, thumbnailSize - 8.0f))) {
              s_TempSelectedHandle = item.handle;
              s_TempSelectedPath = item.relativePath;
            }
          } else {
            std::string fallbackLabel = AssetTypeToString(item.type);
            if (ImGui::Button(fallbackLabel.c_str(), ImVec2(thumbnailSize, thumbnailSize))) {
              s_TempSelectedHandle = item.handle;
              s_TempSelectedPath = item.relativePath;
            }
          }

          if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            s_TempSelectedHandle = item.handle;
            s_TempSelectedPath = item.relativePath;
            requestConfirm = true;
          }

          if (isSelected) {
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
          } else {
            ImGui::PopStyleColor(1);
          }

          DrawTruncatedFilename(item.filename, thumbnailSize);

          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("File: %s\nPath: %s\nUUID: %llu",
                              item.filename.c_str(),
                              item.relativePath.generic_string().c_str(),
                              static_cast<unsigned long long>(item.handle));
          }

          ImGui::EndGroup();
          ImGui::PopID();
        }

        ImGui::EndTable();
      }
    } else {
      // List View
      bool isNoneSelected = (static_cast<uint64_t>(s_TempSelectedHandle) == 0);
      if (ImGui::Selectable("[ None (Clear Assignment) ]", isNoneSelected,
                           ImGuiSelectableFlags_AllowDoubleClick)) {
        s_TempSelectedHandle = 0;
        s_TempSelectedPath.clear();
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
          requestConfirm = true;
        }
      }
      ImGui::Separator();

      for (const auto &item : s_CachedItems) {
        if (!FilterMatches(item)) {
          continue;
        }

        bool isSelected = (s_TempSelectedHandle == item.handle);
        std::string label = item.filename + "##" +
                            std::to_string(static_cast<uint64_t>(item.handle));

        if (ImGui::Selectable(label.c_str(), isSelected,
                             ImGuiSelectableFlags_AllowDoubleClick)) {
          s_TempSelectedHandle = item.handle;
          s_TempSelectedPath = item.relativePath;
          if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            requestConfirm = true;
          }
        }

        ImGui::SameLine(320.0f);
        ImGui::TextDisabled("%s", item.relativePath.generic_string().c_str());

        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("Path: %s\nUUID: %llu",
                            item.relativePath.generic_string().c_str(),
                            static_cast<unsigned long long>(item.handle));
        }
      }
    }

    ImGui::EndChild();

    // Action Footer
    if (ImGui::Button("Import from Disk...")) {
      ImportFromDisk();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Select an external file, copy it into project Assets, and assign it");
    }

    float rightButtonsWidth = 175.0f;
    float availFooterWidth = ImGui::GetContentRegionAvail().x;
    if (availFooterWidth > rightButtonsWidth) {
      ImGui::SameLine(ImGui::GetCursorPosX() + availFooterWidth - rightButtonsWidth);
    } else {
      ImGui::SameLine();
    }

    if (ImGui::Button("Select", ImVec2(80.0f, 0.0f)) || requestConfirm) {
      ConfirmSelection(s_TempSelectedHandle, s_TempSelectedPath);
      ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(80.0f, 0.0f))) {
      Close();
      ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
  }
}

} // namespace Elysium
