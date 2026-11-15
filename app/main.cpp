#include "elypch.hpp"
#include <Elysium.hpp>
#include <EntryPoint.hpp>
#include <ScriptingAPI.hpp>

class Sandbox : public Elysium::Application {
public:
  Sandbox() : Application("Elysium Game Engine — Editor") {
    ELYSIUM_INFO("Sandbox Application initialized");

    auto *editorLayer = new Elysium::EditorLayer();
    auto &scene = editorLayer->GetScene();
    Elysium::SetActiveScene(&scene);
    scene.Clear();

    PushOverlay(editorLayer);
  }

  ~Sandbox() override = default;
};

Elysium::Application *Elysium::CreateApplication() { return new Sandbox(); }
