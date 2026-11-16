#pragma once

#include "Core.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Elysium {

struct FileDialogFilter {
  std::string name;
  std::string spec; // e.g. "png,jpg,jpeg,tga,bmp", "*.png *.jpg", or "*.elyproj"

  FileDialogFilter() = default;
  FileDialogFilter(std::string filterName, std::string filterSpec)
      : name(std::move(filterName)), spec(std::move(filterSpec)) {}
};

/**
 * @brief Cross-platform native file and folder dialogs for Elysium Engine.
 *
 * Implements native platform dialogs on Windows (GetOpenFileName / IFileDialog),
 * Linux (zenity, kdialog, or native portal), and macOS (osascript).
 */
class ELYSIUM_API FileDialog {
public:
  /**
   * @brief Opens a native file dialog to select a single file to open.
   * @param title Title of the dialog window.
   * @param defaultPath Default directory or file path to open at.
   * @param filters List of file extension filters.
   * @return The absolute path of the chosen file, or std::nullopt if cancelled.
   */
  static std::optional<std::filesystem::path> OpenFile(
      const std::string &title = "Open File",
      const std::filesystem::path &defaultPath = "",
      const std::vector<FileDialogFilter> &filters = {});

  /**
   * @brief Opens a native file dialog to select a file path to save.
   * @param title Title of the dialog window.
   * @param defaultPath Default directory or suggested file name.
   * @param filters List of file extension filters.
   * @return The absolute path of the chosen file, or std::nullopt if cancelled.
   */
  static std::optional<std::filesystem::path> SaveFile(
      const std::string &title = "Save File",
      const std::filesystem::path &defaultPath = "",
      const std::vector<FileDialogFilter> &filters = {});

  /**
   * @brief Opens a native folder selection dialog to select an existing directory.
   * @param title Title of the dialog window.
   * @param defaultPath Default directory path.
   * @return The absolute path of the chosen folder, or std::nullopt if cancelled.
   */
  static std::optional<std::filesystem::path> SelectFolder(
      const std::string &title = "Select Folder",
      const std::filesystem::path &defaultPath = "");

  // Cross-platform filter translation helpers
  static std::vector<std::string> ParseFilterSpec(const std::string &spec);
  static std::string FormatLinuxFilterPattern(const std::string &spec, bool includeUppercase = true);
  static std::string FormatWindowsFilterPattern(const std::string &spec);
  static std::vector<std::string> FormatMacOSFilterExtensions(const std::string &spec);
};

} // namespace Elysium
