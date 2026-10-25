#include "FileDialog.hpp"
#include "Log.h"

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

#if defined(ELYSIUM_PLATFORM_LINUX) || defined(__linux__)

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
      cmd << " --filename=\"" << defaultPath.string() << "\"";
    }
    for (const auto &f : filters) {
      cmd << " --file-filter=\"" << f.name << " | " << f.spec << "\"";
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
      cmd << " :";
    }
    if (!filters.empty()) {
      cmd << " \"";
      for (size_t i = 0; i < filters.size(); ++i) {
        if (i > 0) cmd << " ";
        cmd << filters[i].spec;
      }
      cmd << "\"";
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
      cmd << " --filename=\"" << defaultPath.string() << "\"";
    }
    for (const auto &f : filters) {
      cmd << " --file-filter=\"" << f.name << " | " << f.spec << "\"";
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
      cmd << " :";
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
      cmd << " --filename=\"" << defaultPath.string() << "/\"";
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
    } else {
      cmd << " :";
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

#elif defined(ELYSIUM_PLATFORM_WINDOWS) || defined(_WIN32)

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
  for (const auto &f : filters) {
    filterStr += f.name;
    filterStr.push_back('\0');
    filterStr += f.spec;
    filterStr.push_back('\0');
  }
  filterStr += "All Files (*.*)\0*.*\0";
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
  for (const auto &f : filters) {
    filterStr += f.name;
    filterStr.push_back('\0');
    filterStr += f.spec;
    filterStr.push_back('\0');
  }
  filterStr += "All Files (*.*)\0*.*\0";
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
  (void)filters;
  std::string script = "POSIX path of (choose file with prompt \\\"" + title + "\\\")";
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
  (void)filters;
  std::string script = "POSIX path of (choose file name with prompt \\\"" + title + "\\\")";
  std::string out = RunAppleScript(script);
  if (!out.empty()) {
    return std::filesystem::path(out);
  }
  return std::nullopt;
}

std::optional<std::filesystem::path> FileDialog::SelectFolder(
    const std::string &title,
    const std::filesystem::path &defaultPath) {
  std::string script = "POSIX path of (choose folder with prompt \\\"" + title + "\\\")";
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
