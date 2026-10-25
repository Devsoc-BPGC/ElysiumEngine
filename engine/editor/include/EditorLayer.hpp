#ifndef EDITORLAYER_HPP
#define EDITORLAYER_HPP

#include "EditorContext.hpp"
#include "EditorPanel.hpp"
#include "KeyEvent.hpp"
#include "Layer.hpp"
#include "Scene.hpp"
#include "SimpleRenderer.hpp"
#include "Timestep.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>
#include <vector>

namespace Elysium {

class EditorLayer : public Layer {
public:
  EditorLayer();
  ~EditorLayer() override;

  void OnAttach() override;
  void OnDetach() override;
  void OnUpdate(Timestep ts) override;
  void OnImGuiRender() override;
  void OnEvent(Event &event) override;

  void Init(sf::RenderWindow &window);
  void Shutdown();

  EditorContext &GetContext() { return m_context; }
  const EditorContext &GetContext() const { return m_context; }

  Scene &GetScene() { return m_scene; }
  SimpleRenderer &GetRenderer() { return m_renderer; }

  template <typename T, typename... Args> T &AddPanel(Args &&...args) {
    auto panel = std::make_unique<T>(std::forward<Args>(args)...);
    T &ref = *panel;
    m_panels.push_back(std::move(panel));
    return ref;
  }

  bool IsInitialized() const { return m_isInitialized; }

  void SetupDockSpace();
  void RenderMenuBar(Scene &scene);
  void RenderProjectModals();
  void ApplyModernDarkTheme();

  void OnProjectLoaded();
  void NewProjectDialog();
  void OpenProjectDialog();

  bool OnKeyPressed(KeyPressedEvent &e);

  sf::RenderWindow *m_window = nullptr;
  EditorContext m_context;
  std::vector<std::unique_ptr<EditorPanel>> m_panels;
  bool m_isInitialized = false;

  // Project Modals State
  bool m_ShowNewProjectModal = false;
  bool m_ShowOpenProjectModal = false;
  char m_NewProjectNameBuffer[128] = "MyProject";
  char m_NewProjectPathBuffer[512] = "projects/MyProject";
  char m_OpenProjectPathBuffer[512] = "";

  Scene m_scene;
  SimpleRenderer m_renderer{50.0f};
  float m_lastDt = 0.016f;
};

} // namespace Elysium

#endif // EDITORLAYER_HPP
