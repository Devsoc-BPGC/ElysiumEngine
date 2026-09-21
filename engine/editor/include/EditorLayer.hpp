#ifndef EDITORLAYER_HPP
#define EDITORLAYER_HPP

#include "EditorContext.hpp"
#include "EditorPanel.hpp"
#include "Scene.hpp"
#include "SimpleRenderer.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <memory>
#include <vector>

namespace Elysium {

class EditorLayer {
public:
  EditorLayer();
  ~EditorLayer();

  void Init(sf::RenderWindow &window);
  void ProcessEvent(const sf::Event &event);
  void BeginFrame(float dt);
  void RenderPanels(Scene &scene, SimpleRenderer &renderer);
  void EndFrame();
  void Shutdown();

  EditorContext &GetContext() { return m_context; }
  const EditorContext &GetContext() const { return m_context; }

  template <typename T, typename... Args>
  T &AddPanel(Args &&...args) {
    auto panel = std::make_unique<T>(std::forward<Args>(args)...);
    T &ref = *panel;
    m_panels.push_back(std::move(panel));
    return ref;
  }

  bool IsInitialized() const { return m_isInitialized; }

private:
  void SetupDockSpace();
  void RenderMenuBar(Scene &scene);
  void ApplyModernDarkTheme();

  sf::RenderWindow *m_window = nullptr;
  EditorContext m_context;
  std::vector<std::unique_ptr<EditorPanel>> m_panels;
  bool m_isInitialized = false;
};

} // namespace Elysium

#endif // EDITORLAYER_HPP
