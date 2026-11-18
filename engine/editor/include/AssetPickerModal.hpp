#pragma once

#include "AssetManager.hpp"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace Elysium {

/**
 * @brief Reusable, searchable in-editor Dear ImGui asset picker modal.
 *
 * Allows browsing, searching, and selecting assets by AssetType with visual
 * thumbnail grid and list view options, external file import, and direct
 * undoable assignment integration.
 */
class ELYSIUM_API AssetPickerModal {
public:
  using AssetSelectedCallback =
      std::function<void(AssetHandle, const std::filesystem::path &)>;

  static void Open(const std::string &title, AssetType type,
                   AssetHandle currentSelection,
                   AssetSelectedCallback onSelect);
  static void Draw();
  static bool IsOpen();
  static void Close();

private:
  static void RefreshItems();
  static void ImportFromDisk();
  static bool FilterMatches(const AssetItem &item);
  static void ConfirmSelection(AssetHandle handle,
                               const std::filesystem::path &path);

private:
  static bool s_IsOpen;
  static bool s_OpenPopupRequested;
  static bool s_JustOpened;
  static std::string s_Title;
  static AssetType s_TargetType;
  static AssetHandle s_CurrentSelection;
  static AssetHandle s_TempSelectedHandle;
  static std::filesystem::path s_TempSelectedPath;
  static AssetSelectedCallback s_Callback;
  static char s_SearchBuffer[256];
  static bool s_IsGridView;
  static std::vector<AssetItem> s_CachedItems;
};

} // namespace Elysium
