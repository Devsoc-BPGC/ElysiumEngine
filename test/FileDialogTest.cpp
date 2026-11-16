#include "FileDialog.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

using namespace Elysium;

static void Check(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[FAIL] " << message << std::endl;
    std::exit(1);
  }
  std::cout << "[PASS] " << message << std::endl;
}

void TestParseFilterSpec() {
  // 1. Semicolon-delimited (Windows style)
  {
    auto exts = FileDialog::ParseFilterSpec("*.png;*.jpg;*.jpeg;*.tga;*.bmp");
    std::vector<std::string> expected = {"png", "jpg", "jpeg", "tga", "bmp"};
    Check(exts == expected, "ParseFilterSpec handles semicolon-delimited globs");
  }

  // 2. Comma-delimited (extension list)
  {
    auto exts = FileDialog::ParseFilterSpec("png,jpg,jpeg,tga,bmp");
    std::vector<std::string> expected = {"png", "jpg", "jpeg", "tga", "bmp"};
    Check(exts == expected, "ParseFilterSpec handles comma-delimited extensions");
  }

  // 3. Space-delimited (Linux style)
  {
    auto exts = FileDialog::ParseFilterSpec("*.png *.jpg *.jpeg *.tga *.bmp");
    std::vector<std::string> expected = {"png", "jpg", "jpeg", "tga", "bmp"};
    Check(exts == expected, "ParseFilterSpec handles space-delimited globs");
  }

  // 4. Mixed delimiters and whitespace
  {
    auto exts = FileDialog::ParseFilterSpec("*.png; *.jpg, *.jpeg\t*.tga | *.bmp");
    std::vector<std::string> expected = {"png", "jpg", "jpeg", "tga", "bmp"};
    Check(exts == expected, "ParseFilterSpec handles mixed delimiters and whitespace");
  }

  // 5. Single glob / extension
  {
    auto extsA = FileDialog::ParseFilterSpec("*.elyproj");
    Check(extsA == std::vector<std::string>{"elyproj"}, "ParseFilterSpec handles single *.ext glob");

    auto extsB = FileDialog::ParseFilterSpec("elyproj");
    Check(extsB == std::vector<std::string>{"elyproj"}, "ParseFilterSpec handles single extension without dot/star");
  }

  // 6. Wildcard all files
  {
    auto extsStar = FileDialog::ParseFilterSpec("*");
    Check(extsStar == std::vector<std::string>{"*"}, "ParseFilterSpec handles '*' wildcard");

    auto extsStarDotStar = FileDialog::ParseFilterSpec("*.*");
    Check(extsStarDotStar == std::vector<std::string>{"*"}, "ParseFilterSpec handles '*.*' wildcard");
  }
}

void TestFormatLinuxFilterPattern() {
  // Semicolon-delimited input should be translated into space-separated globs with uppercase variants
  std::string pattern = FileDialog::FormatLinuxFilterPattern("*.png;*.jpg;*.jpeg;*.tga;*.bmp", true);

  // Must not contain any semicolons
  Check(pattern.find(';') == std::string::npos, "Linux filter pattern contains no semicolons");

  // Must contain lowercase and uppercase variants
  Check(pattern.find("*.png") != std::string::npos, "Linux pattern includes *.png");
  Check(pattern.find("*.PNG") != std::string::npos, "Linux pattern includes *.PNG");
  Check(pattern.find("*.jpg") != std::string::npos, "Linux pattern includes *.jpg");
  Check(pattern.find("*.JPG") != std::string::npos, "Linux pattern includes *.JPG");
  Check(pattern.find("*.jpeg") != std::string::npos, "Linux pattern includes *.jpeg");
  Check(pattern.find("*.JPEG") != std::string::npos, "Linux pattern includes *.JPEG");
  Check(pattern.find("*.tga") != std::string::npos, "Linux pattern includes *.tga");
  Check(pattern.find("*.TGA") != std::string::npos, "Linux pattern includes *.TGA");
  Check(pattern.find("*.bmp") != std::string::npos, "Linux pattern includes *.bmp");
  Check(pattern.find("*.BMP") != std::string::npos, "Linux pattern includes *.BMP");

  // Wildcard input returns "*"
  Check(FileDialog::FormatLinuxFilterPattern("*.*") == "*", "Linux pattern for '*.*' is '*'");
  Check(FileDialog::FormatLinuxFilterPattern("*") == "*", "Linux pattern for '*' is '*'");
}

void TestFormatWindowsFilterPattern() {
  // Space-delimited input should be translated into semicolon-delimited globs
  std::string pattern = FileDialog::FormatWindowsFilterPattern("*.png *.jpg *.jpeg *.tga *.bmp");
  Check(pattern == "*.png;*.jpg;*.jpeg;*.tga;*.bmp", "Windows filter pattern is semicolon-delimited");

  // Wildcard input returns "*.*"
  Check(FileDialog::FormatWindowsFilterPattern("*") == "*.*", "Windows pattern for '*' is '*.*'");
  Check(FileDialog::FormatWindowsFilterPattern("*.*") == "*.*", "Windows pattern for '*.*' is '*.*'");
}

void TestFormatMacOSFilterExtensions() {
  auto exts = FileDialog::FormatMacOSFilterExtensions("*.png;*.jpg");
  std::vector<std::string> expected = {"png", "PNG", "jpg", "JPG"};
  Check(exts == expected, "macOS filter extensions include lowercase and uppercase");

  auto allFiles = FileDialog::FormatMacOSFilterExtensions("*");
  Check(allFiles.empty(), "macOS filter extensions for '*' returns empty (all files)");
}

int main() {
  std::cout << "Starting FileDialogTest..." << std::endl;
  TestParseFilterSpec();
  TestFormatLinuxFilterPattern();
  TestFormatWindowsFilterPattern();
  TestFormatMacOSFilterExtensions();
  std::cout << "All FileDialogTest assertions PASSED successfully." << std::endl;
  return 0;
}
