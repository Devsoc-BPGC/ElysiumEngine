#include "elypch.hpp"
#include "FileDialog.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#if defined(ELYSIUM_PLATFORM_WINDOWS) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#endif

namespace Elysium {

// ============================================================================
// Cross-Platform Filter Translation Helpers
// ============================================================================

std::vector<std::string> FileDialog::ParseFilterSpec(const std::string &spec) {
  std::vector<std::string> exts;
  std::string token;

  for (size_t i = 0; i <= spec.size(); ++i) {
    char c = (i < spec.size()) ? spec[i] : ';';
    if (c == ';' || c == ',' || c == ' ' || c == '\t' || c == '|' || i == spec.size()) {
      if (!token.empty()) {
        if (token == "*" || token == "*.*" || token == ".*") {
          exts.push_back("*");
        } else {
          size_t start = 0;
          while (start < token.size() && (token[start] == '*' || token[start] == '.' || token[start] == ' ')) {
            start++;
          }
          std::string ext = token.substr(start);
          while (!ext.empty() && (ext.back() == ' ' || ext.back() == '\t')) {
            ext.pop_back();
          }
          if (!ext.empty()) {
            exts.push_back(ext);
          }
        }
        token.clear();
      }
    } else {
      token.push_back(c);
    }
  }

  return exts;
}

std::string FileDialog::FormatLinuxFilterPattern(const std::string &spec, bool includeUppercase) {
  auto exts = ParseFilterSpec(spec);
  if (exts.empty()) {
    return "*";
  }

  std::ostringstream oss;
  bool first = true;
  for (const auto &ext : exts) {
    if (ext == "*") {
      return "*";
    }

    std::string lowerExt = ext;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    std::string upperExt = ext;
    std::transform(upperExt.begin(), upperExt.end(), upperExt.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

    if (!first) oss << " ";
    oss << "*." << lowerExt;
    first = false;

    if (includeUppercase && upperExt != lowerExt) {
      oss << " *." << upperExt;
    }
  }

  return oss.str();
}

std::string FileDialog::FormatWindowsFilterPattern(const std::string &spec) {
  auto exts = ParseFilterSpec(spec);
  if (exts.empty()) {
    return "*.*";
  }

  std::ostringstream oss;
  bool first = true;
  for (const auto &ext : exts) {
    if (ext == "*") {
      return "*.*";
    }

    std::string lowerExt = ext;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (!first) oss << ";";
    oss << "*." << lowerExt;
    first = false;
  }

  return oss.str();
}

std::vector<std::string> FileDialog::FormatMacOSFilterExtensions(const std::string &spec) {
  auto exts = ParseFilterSpec(spec);
  std::vector<std::string> result;

  for (const auto &ext : exts) {
    if (ext == "*") {
      return {}; // Empty indicates all files / no restriction
    }

    std::string lowerExt = ext;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    std::string upperExt = ext;
    std::transform(upperExt.begin(), upperExt.end(), upperExt.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

    result.push_back(lowerExt);
    if (upperExt != lowerExt) {
      result.push_back(upperExt);
    }
  }

  return result;
}

#if defined(ELYSIUM_PLATFORM_WINDOWS) || defined(_WIN32)

std::optional<std::filesystem::path> FileDialog::OpenFile(
    const std::string &title,
    const std::filesystem::path &defaultPath,
    const std::vector<FileDialogFilter> &filters) {

  OPENFILENAMEA ofn;
  CHAR szFile[MAX_PATH] = {0};
  ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
  ofn.lStructSize = sizeof(OPENFILENAMEA);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);

  std::string filterStr;
  bool hasAllFiles = false;
  for (const auto &f : filters) {
    std::string winSpec = FormatWindowsFilterPattern(f.spec);
    if (winSpec == "*.*" || winSpec == "*") {
      hasAllFiles = true;
    }
    filterStr += f.name;
    filterStr.push_back('\0');
    filterStr += winSpec;
    filterStr.push_back('\0');
  }
  if (!hasAllFiles) {
    filterStr += "All Files (*.*)\0*.*\0";
  }
  filterStr.push_back('\0'); // terminate with double-null
  ofn.lpstrFilter = filterStr.c_str();
  ofn.nFilterIndex = 1;

  std::string initialDir = defaultPath.empty() ? "" : defaultPath.string();
  if (!initialDir.empty()) {
    ofn.lpstrInitialDir = initialDir.c_str();
  }
  ofn.lpstrTitle = title.c_str();
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

  if (GetOpenFileNameA(&ofn) == TRUE) {
    return std::filesystem::path(ofn.lpstrFile);
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SaveFile(
    const std::string &title,
    const std::filesystem::path &defaultPath,
    const std::vector<FileDialogFilter> &filters) {

  OPENFILENAMEA ofn;
  CHAR szFile[MAX_PATH] = {0};
  ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
  ofn.lStructSize = sizeof(OPENFILENAMEA);
  ofn.hwndOwner = nullptr;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);

  std::string filterStr;
  bool hasAllFiles = false;
  for (const auto &f : filters) {
    std::string winSpec = FormatWindowsFilterPattern(f.spec);
    if (winSpec == "*.*" || winSpec == "*") {
      hasAllFiles = true;
    }
    filterStr += f.name;
    filterStr.push_back('\0');
    filterStr += winSpec;
    filterStr.push_back('\0');
  }
  if (!hasAllFiles) {
    filterStr += "All Files (*.*)\0*.*\0";
  }
  filterStr.push_back('\0'); // terminate with double-null
  ofn.lpstrFilter = filterStr.c_str();
  ofn.nFilterIndex = 1;

  std::string initialDir = defaultPath.empty() ? "" : defaultPath.string();
  if (!initialDir.empty()) {
    ofn.lpstrInitialDir = initialDir.c_str();
  }
  ofn.lpstrTitle = title.c_str();
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

  if (GetSaveFileNameA(&ofn) == TRUE) {
    return std::filesystem::path(ofn.lpstrFile);
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SelectFolder(
    const std::string &title,
    const std::filesystem::path &defaultPath) {

  BROWSEINFOA bi = {0};
  bi.lpszTitle = title.c_str();
  bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

  LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
  if (pidl != nullptr) {
    CHAR path[MAX_PATH];
    if (SHGetPathFromIDListA(pidl, path)) {
      CoTaskMemFree(pidl);
      return std::filesystem::path(path);
    }
    CoTaskMemFree(pidl);
  }
  return std::nullopt;
}

#elif defined(ELYSIUM_PLATFORM_LINUX) || defined(__linux__)

static std::string RunCommandAndReadOutput(const std::string &cmd) {
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    return "";
  }

  char buffer[1024];
  std::string result;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result += buffer;
  }
  pclose(pipe);

  // Trim trailing newline / carriage return
  while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
    result.pop_back();
  }
  return result;
}

static bool HasZenity() {
  static int status = -1;
  if (status == -1) {
    int ret = std::system("which zenity >/dev/null 2>&1");
    status = (ret == 0) ? 1 : 0;
  }
  return status == 1;
}

static bool HasKdialog() {
  static int status = -1;
  if (status == -1) {
    int ret = std::system("which kdialog >/dev/null 2>&1");
    status = (ret == 0) ? 1 : 0;
  }
  return status == 1;
}

std::optional<std::filesystem::path> FileDialog::OpenFile(
    const std::string &title,
    const std::filesystem::path &defaultPath,
    const std::vector<FileDialogFilter> &filters) {

  if (HasZenity()) {
    std::ostringstream cmd;
    cmd << "zenity --file-selection --title=\"" << title << "\"";
    if (!defaultPath.empty()) {
      std::string pathStr = defaultPath.string();
      if (std::filesystem::is_directory(defaultPath) && !pathStr.empty() && pathStr.back() != '/') {
        pathStr += '/';
      }
      cmd << " --filename=\"" << pathStr << "\"";
    }

    bool hasAllFilesFilter = false;
    for (const auto &f : filters) {
      std::string pattern = FormatLinuxFilterPattern(f.spec, true);
      if (pattern == "*") {
        hasAllFilesFilter = true;
      }
      cmd << " --file-filter=\"" << f.name << " | " << pattern << "\"";
    }

    // Always ensure a fallback "All Files (*)" option is present
    if (!hasAllFilesFilter) {
      cmd << " --file-filter=\"All Files | *\"";
    }

    cmd << " 2>/dev/null";

    std::string out = RunCommandAndReadOutput(cmd.str());
    if (!out.empty()) {
      return std::filesystem::path(out);
    }
    return std::nullopt;
  }

  if (HasKdialog()) {
    std::ostringstream cmd;
    cmd << "kdialog --title \"" << title << "\" --getopenfilename";
    if (!defaultPath.empty()) {
      cmd << " \"" << defaultPath.string() << "\"";
    } else {
      cmd << " \":\"";
    }

    std::ostringstream filterStream;
    bool hasAllFilesFilter = false;
    bool first = true;
    for (const auto &f : filters) {
      std::string pattern = FormatLinuxFilterPattern(f.spec, true);
      if (pattern == "*") {
        hasAllFilesFilter = true;
      }
      if (!first) filterStream << "\n";
      filterStream << pattern << "|" << f.name;
      first = false;
    }
    if (!hasAllFilesFilter) {
      if (!first) filterStream << "\n";
      filterStream << "*|All Files";
    }

    std::string kFilter = filterStream.str();
    if (!kFilter.empty()) {
      cmd << " \"" << kFilter << "\"";
    }
    cmd << " 2>/dev/null";

    std::string out = RunCommandAndReadOutput(cmd.str());
    if (!out.empty()) {
      return std::filesystem::path(out);
    }
    return std::nullopt;
  }

  ELYSIUM_CORE_WARN("FileDialog: Neither zenity nor kdialog is installed on this Linux system.");
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SaveFile(
    const std::string &title,
    const std::filesystem::path &defaultPath,
    const std::vector<FileDialogFilter> &filters) {

  if (HasZenity()) {
    std::ostringstream cmd;
    cmd << "zenity --file-selection --save --confirm-overwrite --title=\"" << title << "\"";
    if (!defaultPath.empty()) {
      std::string pathStr = defaultPath.string();
      if (std::filesystem::is_directory(defaultPath) && !pathStr.empty() && pathStr.back() != '/') {
        pathStr += '/';
      }
      cmd << " --filename=\"" << pathStr << "\"";
    }

    bool hasAllFilesFilter = false;
    for (const auto &f : filters) {
      std::string pattern = FormatLinuxFilterPattern(f.spec, true);
      if (pattern == "*") {
        hasAllFilesFilter = true;
      }
      cmd << " --file-filter=\"" << f.name << " | " << pattern << "\"";
    }

    if (!hasAllFilesFilter) {
      cmd << " --file-filter=\"All Files | *\"";
    }

    cmd << " 2>/dev/null";

    std::string out = RunCommandAndReadOutput(cmd.str());
    if (!out.empty()) {
      return std::filesystem::path(out);
    }
    return std::nullopt;
  }

  if (HasKdialog()) {
    std::ostringstream cmd;
    cmd << "kdialog --title \"" << title << "\" --getsavefilename";
    if (!defaultPath.empty()) {
      cmd << " \"" << defaultPath.string() << "\"";
    } else {
      cmd << " \":\"";
    }

    std::ostringstream filterStream;
    bool hasAllFilesFilter = false;
    bool first = true;
    for (const auto &f : filters) {
      std::string pattern = FormatLinuxFilterPattern(f.spec, true);
      if (pattern == "*") {
        hasAllFilesFilter = true;
      }
      if (!first) filterStream << "\n";
      filterStream << pattern << "|" << f.name;
      first = false;
    }
    if (!hasAllFilesFilter) {
      if (!first) filterStream << "\n";
      filterStream << "*|All Files";
    }

    std::string kFilter = filterStream.str();
    if (!kFilter.empty()) {
      cmd << " \"" << kFilter << "\"";
    }
    cmd << " 2>/dev/null";

    std::string out = RunCommandAndReadOutput(cmd.str());
    if (!out.empty()) {
      return std::filesystem::path(out);
    }
    return std::nullopt;
  }

  ELYSIUM_CORE_WARN("FileDialog: Neither zenity nor kdialog is installed on this Linux system.");
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SelectFolder(
    const std::string &title,
    const std::filesystem::path &defaultPath) {

  if (HasZenity()) {
    std::ostringstream cmd;
    cmd << "zenity --file-selection --directory --title=\"" << title << "\"";
    if (!defaultPath.empty()) {
      std::string pathStr = defaultPath.string();
      if (std::filesystem::is_directory(defaultPath) && !pathStr.empty() && pathStr.back() != '/') {
        pathStr += '/';
      }
      cmd << " --filename=\"" << pathStr << "\"";
    }
    cmd << " 2>/dev/null";

    std::string out = RunCommandAndReadOutput(cmd.str());
    if (!out.empty()) {
      return std::filesystem::path(out);
    }
    return std::nullopt;
  }

  if (HasKdialog()) {
    std::ostringstream cmd;
    cmd << "kdialog --title \"" << title << "\" --getexistingdirectory";
    if (!defaultPath.empty()) {
      cmd << " \"" << defaultPath.string() << "\"";
    }
    cmd << " 2>/dev/null";

    std::string out = RunCommandAndReadOutput(cmd.str());
    if (!out.empty()) {
      return std::filesystem::path(out);
    }
    return std::nullopt;
  }

  ELYSIUM_CORE_WARN("FileDialog: Neither zenity nor kdialog is installed on this Linux system.");
  return std::nullopt;
}

#elif defined(ELYSIUM_PLATFORM_MAC) || defined(__APPLE__)

static std::string RunAppleScript(const std::string &script) {
  std::ostringstream cmd;
  cmd << "osascript -e \"" << script << "\" 2>/dev/null";
  FILE *pipe = popen(cmd.str().c_str(), "r");
  if (!pipe) return "";

  char buffer[1024];
  std::string result;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    result += buffer;
  }
  pclose(pipe);

  while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
    result.pop_back();
  }
  return result;
}

std::optional<std::filesystem::path> FileDialog::OpenFile(
    const std::string &title,
    const std::filesystem::path &defaultPath,
    const std::vector<FileDialogFilter> &filters) {

  std::string script = "POSIX path of (choose file with prompt \\\"" + title + "\\\"";
  if (!defaultPath.empty()) {
    script += " default location POSIX file \\\"" + defaultPath.string() + "\\\"";
  }

  std::vector<std::string> allExts;
  bool hasAllFiles = false;
  for (const auto &f : filters) {
    auto exts = FormatMacOSFilterExtensions(f.spec);
    if (exts.empty()) {
      hasAllFiles = true;
    }
    allExts.insert(allExts.end(), exts.begin(), exts.end());
  }

  if (!allExts.empty() && !hasAllFiles) {
    script += " of type {";
    for (size_t i = 0; i < allExts.size(); ++i) {
      if (i > 0) script += ", ";
      script += "\\\"" + allExts[i] + "\\\"";
    }
    script += "}";
  }
  script += ")";

  std::string out = RunAppleScript(script);
  if (!out.empty()) {
    return std::filesystem::path(out);
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SaveFile(
    const std::string &title,
    const std::filesystem::path &defaultPath,
    const std::vector<FileDialogFilter> &filters) {

  std::string script = "POSIX path of (choose file name with prompt \\\"" + title + "\\\"";
  if (!defaultPath.empty()) {
    script += " default location POSIX file \\\"" + defaultPath.string() + "\\\"";
  }

  std::vector<std::string> allExts;
  bool hasAllFiles = false;
  for (const auto &f : filters) {
    auto exts = FormatMacOSFilterExtensions(f.spec);
    if (exts.empty()) {
      hasAllFiles = true;
    }
    allExts.insert(allExts.end(), exts.begin(), exts.end());
  }

  if (!allExts.empty() && !hasAllFiles) {
    script += " of type {";
    for (size_t i = 0; i < allExts.size(); ++i) {
      if (i > 0) script += ", ";
      script += "\\\"" + allExts[i] + "\\\"";
    }
    script += "}";
  }
  script += ")";

  std::string out = RunAppleScript(script);
  if (!out.empty()) {
    return std::filesystem::path(out);
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SelectFolder(
    const std::string &title,
    const std::filesystem::path &defaultPath) {
  std::string script = "POSIX path of (choose folder with prompt \\\"" + title + "\\\"";
  if (!defaultPath.empty()) {
    script += " default location POSIX file \\\"" + defaultPath.string() + "\\\"";
  }
  script += ")";
  std::string out = RunAppleScript(script);
  if (!out.empty()) {
    return std::filesystem::path(out);
  }
  return std::nullopt;
}

#else

std::optional<std::filesystem::path> FileDialog::OpenFile(
    const std::string &, const std::filesystem::path &, const std::vector<FileDialogFilter> &) {
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SaveFile(
    const std::string &, const std::filesystem::path &, const std::vector<FileDialogFilter> &) {
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SelectFolder(
    const std::string &, const std::filesystem::path &) {
  return std::nullopt;
}

#endif

} // namespace Elysium
