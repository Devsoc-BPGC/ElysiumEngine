#include "elypch.hpp"
#include <Elysium.hpp>
#include <EntryPoint.hpp>
#include <ScriptingAPI.hpp>

#include <algorithm>
#include <cctype>
#include <optional>

namespace {

std::optional<std::filesystem::path>
FindCliProjectArg(const Elysium::ApplicationCommandLineArgs &args) {
  for (int i = 1; i < args.Count; ++i) {
    const char *rawArg = args[i];
    if (!rawArg || rawArg[0] == '\0') {
      continue;
    }
    const std::filesystem::path cliPath(rawArg);
    std::string ext = cliPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) {
                     return static_cast<char>(std::tolower(c));
                   });
    if (ext == ".elyproj") {
      return cliPath;
    }
  }
  return std::nullopt;
}

Elysium::WindowProps
MakeInitialWindowProps(const Elysium::ApplicationCommandLineArgs &args) {
  if (FindCliProjectArg(args).has_value()) {
    return Elysium::WindowProps("Elysium Game Engine — Editor", 1280, 720,
                                true);
  }
  // Compact, non-resizable window so tiling Wayland compositors float it on top
  return Elysium::WindowProps("Elysium Game Engine — Project Hub", 820, 520,
                              false);
}

} // namespace

class Sandbox : public Elysium::Application {
public:
  explicit Sandbox(Elysium::ApplicationCommandLineArgs args)
      : Application(MakeInitialWindowProps(args), args) {
    ELYSIUM_INFO("Sandbox Application initialized");

    auto *editorLayer = new Elysium::EditorLayer();
    auto &scene = editorLayer->GetScene();
    Elysium::SetActiveScene(&scene);
    scene.Clear();

    PushOverlay(editorLayer);

    // Bypass Project Hub when a .elyproj path is provided via command-line args
    if (const auto cliPath = FindCliProjectArg(args)) {
      std::string err;
      if (!editorLayer->OpenProject(*cliPath, &err)) {
        ELYSIUM_ERROR("Failed to open CLI project '{}': {}",
                      cliPath->string(), err);
        if (auto *hub = editorLayer->GetProjectHubPanel()) {
          hub->SetErrorMessage("Failed to open CLI project '" +
                               cliPath->string() + "': " + err);
        }
        // Revert to compact floating Project Hub window if CLI load failed
        editorLayer->m_PendingWindowMode =
            Elysium::EditorLayer::PendingWindowMode::ProjectHub;
      }
    }
  }

  ~Sandbox() override = default;
};

Elysium::Application *
Elysium::CreateApplication(Elysium::ApplicationCommandLineArgs args) {
  return new Sandbox(args);
}
